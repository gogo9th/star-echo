
#include "q2cathedral.h"

#ifdef __cplusplus  
extern "C" {
#endif
#include <libavformat/avformat.h>
#include "libavcodec/avcodec.h"
#include <libswresample/swresample.h>
#include <libavutil/channel_layout.h>
#include <libavutil/audio_fifo.h>
#ifdef __cplusplus  
}
#endif

#include <variant>

#include <boost/algorithm/string.hpp>

#include "log.hpp"

#include "mediaProcess.h"


av_always_inline std::string av_err2string(int errnum)
{
    char str[AV_ERROR_MAX_STRING_SIZE];
    return av_make_error_string(str, AV_ERROR_MAX_STRING_SIZE, errnum);
}


template<typename T>
class scoped_ptr : public std::unique_ptr<T, void(*)(T *)>
{
public:
    using deleter = void(*)(T *);

    scoped_ptr(T * v, const deleter & d) : std::unique_ptr<T, deleter>(v, d) {};

    operator T * () const { return this->get(); }
};


class MPError : public std::runtime_error
{
public:
    MPError(const char * msg, bool isError = true)
        : runtime_error(msg)
        , isError_(isError)
    {}

    MPError(const char * msg, int errorCode)
        : runtime_error(std::string(msg) + " (" + std::to_string(errorCode) + ") - " + av_err2string(errorCode))
    {}

    bool isError() const { return isError_; }

protected:
    bool isError_ = false;
};

//

static scoped_ptr<AVCodecContext> createCodec(AVCodecParameters * codecpar,
                                              AVSampleFormat desiredFormat = AV_SAMPLE_FMT_NONE,
                                              bool output = false)
{
    if (!codecpar)
    {
        return scoped_ptr<AVCodecContext>(nullptr, [] (auto d) {});
    }

    int r;

    const AVCodec * codec = output ? avcodec_find_encoder(codecpar->codec_id) : avcodec_find_decoder(codecpar->codec_id);
    if (!codec) throw MPError("failed to find codec");

    scoped_ptr<AVCodecContext> ctx(avcodec_alloc_context3(codec),
                                   [] (auto d) { avcodec_free_context(&d); });
    if (!ctx) throw MPError("failed to allocate codec");

    // set the codec's parameters to the ctx
    if ((r = avcodec_parameters_to_context(ctx, codecpar)) < 0)
        throw MPError("avcodec_parameters_to_context", r);

    // workaround to merge default codec format
    if (codecpar->format == AV_SAMPLE_FMT_NONE)
    {
        if (desiredFormat == AV_SAMPLE_FMT_NONE)
        {
            ctx->sample_fmt = codec->sample_fmts[0];
        }
        else
        {
            AVSampleFormat suitable = AV_SAMPLE_FMT_NONE;
            AVSampleFormat suitableP = AV_SAMPLE_FMT_NONE;
            auto * fmt = codec->sample_fmts;
            while (*fmt != AV_SAMPLE_FMT_NONE)
            {
                if (*fmt == desiredFormat)
                {
                    ctx->sample_fmt = *fmt;
                    break;
                }
                if (av_get_bytes_per_sample(*fmt) == av_get_bytes_per_sample(desiredFormat))
                {
                    if (av_sample_fmt_is_planar(*fmt) == av_sample_fmt_is_planar(desiredFormat))
                    {
                        suitableP = *fmt;
                    }
                    else
                    {
                        suitable = *fmt;
                    }
                }
                ++fmt;
            }

            if (ctx->sample_fmt == AV_SAMPLE_FMT_NONE)
            {
                if (suitableP != AV_SAMPLE_FMT_NONE)
                {
                    ctx->sample_fmt = suitableP;
                }
                else if (suitable != AV_SAMPLE_FMT_NONE)
                {
                    ctx->sample_fmt = suitable;
                }
                else
                {
                    ctx->sample_fmt = codec->sample_fmts[0];
                }
            }
        }
    }
    if (codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
    {
        ctx->time_base.den = codecpar->sample_rate > 0 ? codecpar->sample_rate : 1;
        ctx->time_base.num = 1;
    }

    if ((r = avcodec_open2(ctx, codec, NULL)) != 0)
        throw MPError("failed to open codec", r);

    if (ctx->ch_layout.nb_channels == 0)
        av_channel_layout_default(&ctx->ch_layout, codecpar->ch_layout.nb_channels);
    return ctx;
}

static int encodeFrame(AVFrame * frame, AVFormatContext * avfmt_out, AVCodecContext * avcodec_out, int streamIndex)
{
    int r = avcodec_send_frame(avcodec_out, frame);
    if (r != 0 && r != AVERROR_EOF)
    {
        throw MPError("failed to send frame", r);
    }

    scoped_ptr<AVPacket> packet(av_packet_alloc(), [] (auto * d) { av_packet_free(&d); });

    if ((r = avcodec_receive_packet(avcodec_out, packet)) == 0)
    {
        //if (frame && packet.dts == 0)
        //{
        //    packet.pts = frame->pts;
        //    packet.dts = frame->pts;
        //}
        // printf(" * %i\n", packet.dts);

        packet->stream_index = streamIndex;

        // <TIP: write a filtered raw frame to the output file with a proper codec-encoding>
        // <FLOW: AVFrame frame -> AVCodecContext avcodec_out -> AVPacket packet -> AVFormatContext avfmt_out -> FILE outfile >
        // Write a (codec-processed) packet to an output media file
        if ((r = av_write_frame(avfmt_out, packet)) == 0)
        {
            //return r;
        }
        else
        {
            throw MPError("failed to write frame", r);
        }
    }
    // When we got EOF, there might be still remaining packets in AVCodecContext avcodec_out not encoded, yet
    else if (r == AVERROR(EAGAIN))
    {
        return 0;
    }
    else if (r != AVERROR_EOF)
    {
        throw MPError("failed to receive an output packet", r);
    }

    return r;
}

static void writeToFifo(AVAudioFifo * fifo, void ** buffers, int nbSamples)
{
    int r;

    if ((r = av_audio_fifo_realloc(fifo, av_audio_fifo_size(fifo) + nbSamples)) < 0)
        throw MPError("failed to resize fifo");

    if (av_audio_fifo_write(fifo, buffers, nbSamples) < nbSamples)
        throw MPError("failed to write to fifo");
}

//

template<typename sample_t>
struct FType0
{
    std::vector<sample_t> lb, rb;
    sample_t * lbIn, * rbIn;
    std::vector<sample_t> lbOut, rbOut;

    FType0()
    {
        lbIn = lb.data();
        rbIn = rb.data();
    }

    void resizeIn(size_t size)
    {
        if (lb.size() < size)
        {
            lb.resize(size);
            lbIn = lb.data();
        }
        if (rb.size() < size)
        {
            rb.resize(size);
            rbIn = rb.data();
        }
    }

    void resizeOut(size_t size)
    {
        if (lbOut.size() < size) lbOut.resize(size);
        if (rbOut.size() < size) rbOut.resize(size);
    }

    std::array<uint8_t *, 2> buffersIn() const
    {
        return { (uint8_t *)lb.data(), (uint8_t *)rb.data() };
    }

    std::array<uint8_t *, 2> buffersOut() const
    {
        return { (uint8_t *)lbOut.data(), (uint8_t *)rbOut.data() };
    }

    void buffersExt(uint8_t * d0, uint8_t * d1, int samples)
    {
        lbIn = (sample_t *)d0;
        rbIn = (sample_t *)d1;

        // Prepare room for the filter
        if (lb.size() < samples) lb.resize(samples);
        if (rb.size() < samples) rb.resize(samples);
    }

    template<typename Filter, std::enable_if_t<std::is_same_v<typename Filter::sample_t, sample_t>, bool> = true>
    void run(Filter & filter, int samples)
    {
        filter.filter(lbIn, rbIn, lbOut.data(), rbOut.data(), samples);
    }

    template<typename Filter, std::enable_if_t<!std::is_same_v<typename Filter::sample_t, sample_t>, bool> = true>
    void run(Filter & filter, int samples)
    {}

    void swap()
    {
        lb.swap(lbOut);
        rb.swap(rbOut);
        lbIn = lb.data();
        rbIn = rb.data();
    }
};

//

#if defined(_WIN32)
std::wstring
#else
std::string
#endif
MediaProcess::operator()(const FileItem & item) const
{
    try
    {
        process(item);
        // libc cannot into wstring https://gcc.gnu.org/bugzilla/show_bug.cgi?id=102839
    #if defined(_WIN32)
        return item.output.wstring() + L" (FINISHED)";
    }
    catch (const MPError & e)
    {
        if (e.isError())
        {
            return item.output.wstring() + L" failed : " + stringToWstring(e.what());
        }
        else
        {
            return item.output.wstring() + L" : " + stringToWstring(e.what());
        }
    }
    catch (const std::exception & e)
    {
        return item.output.wstring() + L" exception : " + stringToWstring(e.what());
    }
    #else
        return item.output.string() + " (FINISHED)";
    }
    catch (const MPError & e)
    {
        if (e.isError())
        {
            return item.output.string() + " failed : " + e.what();
        }
        else
        {
            return item.output.string() + " : " + e.what();
        }
    }
    catch (const std::exception & e)
    {
        return item.output.string() + " exception : " + e.what();
    }
#endif
}


void MediaProcess::process(const FileItem & item) const
{
    std::vector<float> normalizers;
    bool ripped = false;
    do {
        if (!do_process(item, normalizers, ripped))
            break;
    } while (ripped);
}


bool MediaProcess::do_process(const FileItem & item, std::vector<float> & normalizers, bool & ripped) const
{
    //#if defined(_DEBUG)
    //    msg() << item.output.string();
    //#endif // defined(_DEBUG)
    bool normalize = item.normalize;

    std::filesystem::create_directories(item.output.parent_path());

    std::filesystem::path tempOut(item.output);
    tempOut += ".tmp";

    int r;

    scoped_ptr<AVFormatContext> avfmt_in([&item] ()
                                         {	// open the input music file
                                             AVFormatContext * ctx = nullptr;
                                             int r = avformat_open_input(&ctx, item.input.string().c_str(), NULL, NULL);
                                             if (r != 0) throw MPError("failed to open input media", r);
                                             return ctx;
                                         }(),
                                             [] (auto * d)
                                         {
                                             avformat_close_input(&d);/* avformat_free_context(d); */
                                         });

    // ensure a stream exists in the input music file
    if ((r = avformat_find_stream_info(avfmt_in, NULL)) < 0)
        throw MPError("no media stream", r);

    //
    //av_dump_format(avfmt_in, 0, "*", false);
    //

    AVStream * audioStream = 0;
    AVStream * imageStream = 0;
    for (int i = 0; i < avfmt_in->nb_streams; i++)
    {
        auto & stream = avfmt_in->streams[i];

        // there might be several attaches pic streams, so we pick the last?
        if (stream->disposition & AV_DISPOSITION_ATTACHED_PIC)
        {
            imageStream = stream;
        }

        if (stream->codecpar->codec_type == AVMEDIA_TYPE_AUDIO)
        {
            if (audioStream) throw MPError("using multiple audio streams is not supported");

            audioStream = stream;
        }
    }
    if (!audioStream) throw MPError("no audio stream is found");

    auto audioCodecIn = createCodec(audioStream->codecpar);
    const int srcChannels = audioCodecIn->ch_layout.nb_channels;
    auto imageCodecIn = createCodec(imageStream ? imageStream->codecpar : nullptr);

    // *** Set up the input format ctx ***

    std::variant<
        // ! update filterFormat and params.codec_id !
        //std::vector<std::unique_ptr<Filter<int16_t, int32_t>>>,
        std::vector<std::unique_ptr<Filter<int32_t, int64_t>>>,
        std::vector<std::unique_ptr<Filter<float, double>>>
    > filters;

    AVSampleFormat filterFormat;
    if (audioCodecIn->sample_fmt == AV_SAMPLE_FMT_FLT || audioCodecIn->sample_fmt == AV_SAMPLE_FMT_FLTP
        || audioCodecIn->sample_fmt == AV_SAMPLE_FMT_DBL || audioCodecIn->sample_fmt == AV_SAMPLE_FMT_DBLP)
    {
        filters = filterFab_.create<float, double>();
        filterFormat = AV_SAMPLE_FMT_FLTP;
    }
    else
    {
        filters = filterFab_.create<std::variant_alternative_t<0, decltype(filters)>::value_type::element_type::sample_t,
                                    std::variant_alternative_t<0, decltype(filters)>::value_type::element_type::samplew_t>();
        //filterFormat = AV_SAMPLE_FMT_S16P;
        filterFormat = AV_SAMPLE_FMT_S32P;
    }


    int filterSampleRate = audioCodecIn->sample_rate;

    std::visit([&filterSampleRate] (auto && filters)
               {
                   for (auto & filter : filters)
                   {
                       filterSampleRate = filter->agreeSamplerate(filterSampleRate);
                   }
               }, filters);
    std::visit([&filterSampleRate] (auto && filters)
               {
                   for (auto & filter : filters)
                   {
                       filter->setSamplerate(filterSampleRate);
                   }
               }, filters);

    std::visit([&normalizers] (auto && filters)
               {
                   for (size_t i = 0; i < normalizers.size(); i++)
                       filters[i]->setNormFactor(normalizers[i]);
               }, filters);


    // SwrContext contains the audio file's sound quality parameters, such as the audio channel left/right flags (mono/stereo), 
    //  sampling format (e.g., 16 bits), and sampling rate (e.g., 44kHz)
    scoped_ptr<SwrContext> swr_in(nullptr, [] (SwrContext * d) { swr_free(&d); });
    {

        /* Force the input audio channel to have front left & right */
        /* Force the sampling format to be signed-X-bit-planar */
        /* Force the sampling rate to be agreed sample rate */

        if (audioCodecIn->ch_layout.nb_channels != 2
            || audioCodecIn->sample_fmt != filterFormat
            || (audioCodecIn->sample_rate != filterSampleRate))
        {
            swr_in.reset(swr_alloc());
            if (!swr_in) throw MPError("swr_alloc failed");

            AVChannelLayout stereoLayout = AV_CHANNEL_LAYOUT_STEREO;
            auto swr_in_ = swr_in.get();
            // Arguments: (swr_ctx, out, out, out, in, in, in, log_offset, log_ctx)
            r = swr_alloc_set_opts2(&swr_in_,
                                    &stereoLayout, filterFormat, filterSampleRate,
                                    &audioCodecIn->ch_layout, audioCodecIn->sample_fmt, audioCodecIn->sample_rate,
                                    0, NULL);
            if (r != 0) throw MPError("swr_alloc_set_opts2 (in) failed", r);

            if ((r = swr_init(swr_in)) != 0) throw MPError("input converter init failed", r);
        }
    }

    // *** output format ctx **

    scoped_ptr<AVFormatContext> avfmt_out([&item, &tempOut] ()
                                          {
                                              int r;
                                              AVFormatContext * ctx;
                                              if ((r = avformat_alloc_output_context2(&ctx, 0, 0, item.output.string().c_str())) < 0)
                                                  throw MPError("failed to allocate output format context", r);
                                              return ctx;
                                          }(),
                                              [] (auto * d)
                                          {
                                              avio_closep(&d->pb);
                                              avformat_free_context(d);
                                          }
                                          );

    if ((r = avio_open(&avfmt_out->pb, tempOut.string().c_str(), AVIO_FLAG_WRITE)) < 0)
        throw MPError("failed to open output media", r);

    // Tentatively set up the output audio codec paramaters based on the input audio codec parameters
    AVCodecParameters params {};
    av_channel_layout_default(&params.ch_layout, 2);
    params.sample_rate = filterSampleRate;
    params.bit_rate = audioCodecIn->bit_rate;
    params.codec_type = AVMEDIA_TYPE_AUDIO;
    params.format = AV_SAMPLE_FMT_NONE;
    params.codec_id = avfmt_out->oformat->audio_codec;

    if (params.codec_id == AV_CODEC_ID_FIRST_AUDIO) // that is pcm
    {
        if (filterFormat == AV_SAMPLE_FMT_FLTP)
        {
            params.codec_id = AV_CODEC_ID_PCM_F32LE;
        }
        else
        {
            params.codec_id = AV_CODEC_ID_PCM_S32LE;
            //params.codec_id = AV_CODEC_ID_PCM_S16LE;
        }
    }

    // returns AVCodecContext*
    auto audioCodecOut = createCodec(&params, filterFormat, true);

    if (audioCodecOut->frame_size == 0)
    {
        audioCodecOut->frame_size = audioCodecIn->frame_size;
    }

    //if (imageStream)
    //{
    //    // tweak timebase
    //    imageStream->codecpar->sample_rate = imageStream->time_base.den;
    //}
    //auto imageCodecOut = createCodec(imageStream ? imageStream->codecpar : nullptr, true);


    // Some container formats (like MP4) require global headers to be present.
    // Mark the encoder so that it behaves accordingly.
    if (avfmt_out->oformat->flags & AVFMT_GLOBALHEADER)
        audioCodecOut->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

    // when multiple streams are here, packet stream index should be set correspondingly
    int audioStreamOutIndex;
    int imageStreamOutIndex;

    {
        AVStream * stream_out = 0;
        if (!(stream_out = avformat_new_stream(avfmt_out, audioCodecOut->codec)))
            throw MPError("failed to create output stream (audio)");

        //stream_out->time_base.den = filterSampleRate;
        //stream_out->time_base.num = 1;

        if ((r = avcodec_parameters_from_context(stream_out->codecpar, audioCodecOut)) < 0)
            throw MPError("failed to initialize output stream parameters", r);

        audioStreamOutIndex = stream_out->index;
    }

    if (imageStream) // for the cover image if exists
    {
        AVStream * stream_out = 0;
        if (!(stream_out = avformat_new_stream(avfmt_out, nullptr/*imageCodecOut->codec*/)))
            throw MPError("failed to create output stream (image)");

        if ((r = avcodec_parameters_from_context(stream_out->codecpar, imageCodecIn /*imageCodecOut*/)) < 0)
            throw MPError("failed to initialize output stream parameters", r);

        stream_out->disposition = AV_DISPOSITION_ATTACHED_PIC;
        imageStreamOutIndex = stream_out->index;
    }

    // *** output converter ***

    // overall:
    // in -> S16P/2/FilterSR -> filter -> out/2/OutSR -> fifo -> out

    scoped_ptr<SwrContext> swr_out(nullptr, [] (SwrContext * d) { swr_free(&d); });

    // If the output codec parameter is not what we want, then create awr_out as a format converter
    if (audioCodecOut->sample_fmt != filterFormat)
    {
        swr_out.reset(swr_alloc());
        if (!swr_out) throw MPError("swr_alloc failed");

        AVChannelLayout stereoLayout = AV_CHANNEL_LAYOUT_STEREO;
        auto swr_out_ = swr_out.get();

        r = swr_alloc_set_opts2(&swr_out_,
                                &stereoLayout, audioCodecOut->sample_fmt, filterSampleRate,
                                &stereoLayout, filterFormat, filterSampleRate,
                                0, NULL);
        if (r != 0) throw MPError("swr_alloc_set_opts2 (out) failed", r);

        if ((r = swr_init(swr_out)) != 0) throw MPError("output converter init failed", r);
    }

    //
    //av_dump_format(avfmt_out, 0, "*", true);
    //

    // copy metatada / tags
    r = av_dict_copy(&avfmt_out->metadata, avfmt_in->metadata, 0);
    if (r != 0)
    {
        msg() << item.input.string() << " failed to copy metadata: " << r;
    }

    if ((r = avformat_write_header(avfmt_out, NULL)) < 0)
        throw MPError("failed to write the output file header");


    // *** process ***

    scoped_ptr<AVFrame> frame_in(av_frame_alloc(), [] (auto * d) { av_frame_free(&d); });
    if (!frame_in) throw MPError("failed to allocate frame");

    scoped_ptr<AVAudioFifo> fifo(av_audio_fifo_alloc(audioCodecOut->sample_fmt, audioCodecOut->ch_layout.nb_channels, std::max(audioCodecOut->frame_size, 1)),
                                 [] (auto * d) { av_audio_fifo_free(d); });
    if (!fifo) throw MPError("failed to allocate fifo");


    FType0<std::variant_alternative_t<0, decltype(filters)>::value_type::element_type::sample_t> ftype0;
    FType0<std::variant_alternative_t<1, decltype(filters)>::value_type::element_type::sample_t> ftype1;

    int pts = 0;
    auto Pts = [&pts] (int samples)
    {
        auto r = pts;
        pts += samples;
        return r;
    };

    // Read one audio frame from the input file into a temporary packet.
    bool input_eof = false;
    int silence = filterFab_.getSilence();
    bool is_silence_handled = (silence == 0);
    int silence_samples = silence * filterSampleRate;
    while (!input_eof)
    {
        {
            scoped_ptr<AVPacket> packet(av_packet_alloc(), [] (auto * d) { av_packet_free(&d); });

        readFrame:
            // Read an encoded frame from the input music file
            r = av_read_frame(avfmt_in, packet);
            if (r == 0)
            {   // If this frame is a cover image frame, 
                //  write it directly to the output music file (AVFormatCtx ctx) without decoding/encoding
                if (imageStream && (imageStream->index == packet->stream_index))
                {
                    // <TIP: Get a cover image frame from an input music file and write it to the ouptut music file>
                    // <FLOW: AVFormatContext (avfmt_in) -> AVPacket (packet) -> AVFormatContext (avfmt_out) > 
                    // Write the image stream and continue to read the next frame from the input music file
                    packet->stream_index = imageStreamOutIndex;
                    r = av_write_frame(avfmt_out, packet);
                    av_packet_unref(packet);
                    if (r != 0) throw MPError("failed to write image frame", r);

                    goto readFrame;
                }
                // If this frame is an audio frame, send it to the input audioc codec (audioCodecIn) to decode it 
                else if (audioStream->index == packet->stream_index)
                {
                    r = avcodec_send_packet(audioCodecIn, packet);
                    if (r == 0)
                    {   // <TIP: get a raw frame from the input music file with a proper codec-decoding>
                        // <FLOW: AVFormatContext (avfmt_in) -> AVPacket (packet) -> AVCodexContext (audioCodecIn) 
                        //   -> AVFrame (frame_in) >
                        r = avcodec_receive_frame(audioCodecIn, frame_in);
                        if (r == 0 || r == AVERROR_EOF)
                        {
                            // return r;
                        }
                        else if (r == AVERROR(EAGAIN))
                        {
                            goto readFrame;
                        }
                        else
                        {
                            throw MPError("failed to receive frame", r);
                        }
                    }
                    else if (r == AVERROR_INVALIDDATA
                             && (audioCodecIn->codec_id == AV_CODEC_ID_MP3
                                 || audioCodecIn->codec_id == AV_CODEC_ID_WMAV1 || audioCodecIn->codec_id == AV_CODEC_ID_WMAV2))
                    {
                        // MP3 https://trac.ffmpeg.org/ticket/7879
                        goto readFrame;
                    }
                    else if (r == AVERROR(EPERM)
                             && (audioCodecIn->codec_id == AV_CODEC_ID_WMAV1 || audioCodecIn->codec_id == AV_CODEC_ID_WMAV2))
                    {
                        // WMA https://trac.ffmpeg.org/ticket/9358
                        goto readFrame;
                    }
                    else
                    {
                        throw MPError("failed to send packet (input)", r);
                    }
                }
                else
                {
                    // unknown stream index
                    continue;
                }
            }
        }
        // If some non-zero sample size of the input frame is successfully decoded 
        if (r != AVERROR_EOF && frame_in->nb_samples > 0 || !is_silence_handled)
        {  int nb_samples;
           if (r != AVERROR_EOF && frame_in->nb_samples > 0) 
           {    // If the current channel's layout id is different than the one specified in the input codec
					if (frame_in->ch_layout.nb_channels != srcChannels)
					{
						 // Oh, the channel layout can dynamically change in the middle lol
						 throw MPError("channel layout had changed in the middle");
					}

					// This is the input music file's decoded raw frame
					nb_samples = frame_in->nb_samples;

					// If the input music file is NOT in the format we want (44kHz stereo, signe 16 bits, planar)
					if (swr_in)
					{
						 // frame -> l/rb

						 // An upper bound on the number of samples that the next swr_convert will output
						 auto swrOutSamples = swr_get_out_samples(swr_in, nb_samples);

						 // Dynamically increase the lb, rb vector size to be the maximum number of samples 
						 //  across all decoded raw frames of the input music file
						 filters.index() ? ftype1.resizeIn(swrOutSamples) : ftype0.resizeIn(swrOutSamples);

						 auto buffers = (filters.index() ? ftype1.buffersIn() : ftype0.buffersIn());

						 // Convert the input music file's decoded raw frame to be our desired format (signed 16-bits, planar, 44100 fps)
						 nb_samples = swr_convert(swr_in, buffers.data(), swrOutSamples, (const uint8_t **)frame_in->data, nb_samples);
						 // cannot // assert(nb_samples == cwrOutSamples);

						 // 128 bytes gap of Q2 ClaStr filter output to CH
						 //if (isFirstChunk)
						 //{
						 //    nb_samples += 128;
						 //    lb.insert(lb.begin(), 128, 0);
						 //    rb.insert(rb.begin(), 128, 0);
						 //    isFirstChunk = false;
						 //}
					}
					// If the input music file is in the format we want
					else
					{
						 filters.index() ? ftype1.buffersExt(frame_in->data[0], frame_in->data[1], nb_samples) : ftype0.buffersExt(frame_in->data[0], frame_in->data[1], nb_samples);
					}
           }
           else
           {     nb_samples = silence_samples; 
                 filters.index() ? ftype1.resizeIn(nb_samples) : ftype0.resizeIn(nb_samples);
                 auto in_planes_vec = (filters.index() ? ftype1.buffersIn() : ftype0.buffersIn());
                 uint8_t **in_planes = in_planes_vec.data(); 
                 const int n_channels = 2; // you force stereo 
                 audioCodecOut->sample_fmt; 
                 av_samples_set_silence(in_planes, /*offset=*/0, nb_samples, n_channels, filterFormat); 
                 filters.index() ? ftype1.resizeOut(nb_samples) : ftype0.resizeOut(nb_samples);
                 is_silence_handled = true;
           }

            // Filter-processing

            // convert can produce no samples ... if the input is like 1 sample ... uhhh 
            if (nb_samples > 0)
            {
                // These are for output data (after filter-processing)
                (filters.index() ? ftype1.resizeOut(nb_samples) : ftype0.resizeOut(nb_samples));

                std::visit([&ftype0, &ftype1, &filters, nb_samples] (auto && fs)
                           {
                                for (auto filter = fs.begin(); filter != fs.end(); ++filter)
                                {
                                    filters.index() ? ftype1.run(**filter, nb_samples) : ftype0.run(**filter, nb_samples);

                                    // Reuse the same lb, rb, lbOut, rbOut buffers when processing through multiple filters
                                    if (filter + 1 != fs.end())
                                    {
                                        filters.index() ? ftype1.swap() : ftype0.swap();
                                    }
                                }
                           }, filters);

                // An extra processing to make the output frame to be our desired 
                //  channel layout, sampling rate, sample format (44kHz/48kHz/32kHz stereo s16p)
                if (swr_out)
                {
                    // l/rbOut -> fifo buffers

                    auto swrOutSamples = swr_get_out_samples(swr_out, nb_samples);
                    auto channels = audioCodecOut->ch_layout.nb_channels;
                    auto bs = av_samples_get_buffer_size(0, channels, swrOutSamples, audioCodecOut->sample_fmt, 0);

                    // Resize to required size but send both buffers even for interleaved
                    filters.index() ? ftype1.resizeIn(bs) : ftype0.resizeIn(bs);

                    auto toConvert = (filters.index() ? ftype1.buffersOut() : ftype0.buffersOut());
                    auto fromConvert = (filters.index() ? ftype1.buffersIn() : ftype0.buffersIn());

                    nb_samples = swr_convert(swr_out, fromConvert.data(), bs, (const uint8_t **)toConvert.data(), nb_samples);
                    writeToFifo(fifo, (void **)fromConvert.data(), nb_samples);
                }
                // If the output is already of suitable format
                else
                {
                    auto buffers = (filters.index() ? ftype1.buffersOut() : ftype0.buffersOut());
                    writeToFifo(fifo, (void **)buffers.data(), nb_samples);
                }
            }
        }
        else
        {
            input_eof = true;
            r = 0;
        }

    fflushFifo:

        const int samplesAvail = av_audio_fifo_size(fifo);
        // The frameSize is max chunk we can send to the output codec and this can be less than the filtered available frame data
        const int frameSize = audioCodecOut->frame_size > 0 ? audioCodecOut->frame_size : samplesAvail;

        // If no frame has been written and there is no available output frame, then this is an empty file, so finish
        if (frameSize == 0 || samplesAvail == 0)
        {
            break;
        }

        // When the frame is the last one, the available size is equal to or less than the frame size
        // When the frame is not the last one, the available size is more than the frame size
        if (samplesAvail >= frameSize || input_eof)
        {
            scoped_ptr<AVFrame> frame_out(av_frame_alloc(),
                                          [] (auto * d) { av_frame_free(&d); });
            if (!frame_out) throw MPError("failed to allocate an output frame");

            frame_out->format = audioCodecOut->sample_fmt;
            av_channel_layout_copy(&frame_out->ch_layout, &audioCodecOut->ch_layout);
            frame_out->sample_rate = audioCodecOut->sample_rate;

            do
            {   // This is correct number of maximum samples to read for both the last and non-last frames
                frame_out->nb_samples = std::min(samplesAvail, frameSize);
                // Set a timestamp (i.e., the total elapsed time) based on the sample rate of the container.
                frame_out->pts = Pts(frame_out->nb_samples);

                if (!av_frame_is_writable(frame_out))
                {
                    // We're in a while loop, so reuse the buffer we have in frame_out
                    if ((r = av_frame_get_buffer(frame_out, 0)) < 0)
                        throw MPError("av_frame_get_buffer");
                }
                // Read the filtered frame of the exact size we expect
                if (av_audio_fifo_read(fifo, (void **)frame_out->data, frame_out->nb_samples) < frame_out->nb_samples)
                    throw MPError("failed to read from fifo");

                // Write the frame to the output music file
                if ((r = encodeFrame(frame_out, avfmt_out, audioCodecOut, audioStreamOutIndex)) != 0)
                {
                    break;
                }
            } while (av_audio_fifo_size(fifo) >= frameSize);
        }
    }

    if (r == 0)
    {
        // Encode all unencoded remaining data and flush them to the output music file
        do {} while (encodeFrame(NULL, avfmt_out, audioCodecOut, audioStreamOutIndex) == 0);

        // Flush all encoded remaining data to the output music file
        if ((r = av_write_trailer(avfmt_out)) < 0)
            throw MPError("failed to write output the file trailer", r);

        audioCodecOut.reset();
        avfmt_out.reset();

        ripped = false;
        if (normalize)
        {
            std::visit([&normalizers, &ripped] (auto && filters)
                       {
                           bool is_initial = normalizers.empty();

                           int i = 0;
                           for (auto & filter : filters)
                           {
                               if (is_initial)
                                   normalizers.push_back(filter->normFactor());

                               // normalize only the first ripping filter 
                               if (ripped)
                               {
                                   continue;
                               }

                               // compute (or update) the normalization factor
                               auto newFactor = filter->calcNormFactor();
                               if (newFactor > 1.0f)
                               {
                                   ripped = true;
                                   normalizers[i] *= newFactor;

                                   msg() << "- Normalizing Filter[" << i << "]: Division Factor = " << normalizers[i];
                               }
                               i++;
                           }
                       }, filters);
        }
        // save the music file only if there is no ripping sound
        if (!ripped)
            std::filesystem::rename(tempOut, item.output);
    }
    return r == 0;
}
