
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

#include <boost/algorithm/string.hpp>

#include "mediaProcess.h"
#include "log.hpp"
#include "DNSE_CH.h"
#include "DNSE_EQ.h"


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

static scoped_ptr<AVCodecContext> createCodec(AVCodecParameters * codecpar, bool output = false)
{
    if (!codecpar)
    {
        return scoped_ptr<AVCodecContext>(nullptr, [] (auto d) {});
    }

    int r;

    AVCodec * codec = output ? avcodec_find_encoder(codecpar->codec_id) : avcodec_find_decoder(codecpar->codec_id);
    if (!codec) throw MPError("failed to find codec");

    scoped_ptr<AVCodecContext> ctx(avcodec_alloc_context3(codec),
                                   [] (auto d) { avcodec_free_context(&d); });
    if (!ctx) throw MPError("failed to allocate codec");

    // set the codec's parameters to the ctx
    if ((r = avcodec_parameters_to_context(ctx, codecpar)) < 0)
        throw MPError("avcodec_parameters_to_context", r);

    // workaround to merge default codec format
    if (codecpar->format < 0)
    {
        ctx->sample_fmt = codec->sample_fmts[0];
    }
    if (codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
    {
        ctx->time_base.den = codecpar->sample_rate > 0 ? codecpar->sample_rate : 1;
        ctx->time_base.num = 1;
    }

    if ((r = avcodec_open2(ctx, codec, NULL)) != 0)
        throw MPError("failed to open codec", r);

    if (ctx->channel_layout == 0)
    {
        ctx->channel_layout = av_get_default_channel_layout(codecpar->channels);
    }

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

std::string MediaProcess::operator()(const FileItem & item) const
{
    try
    {
        process(item);
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
}


void MediaProcess::process(const FileItem & item) const
{
    std::vector<float> normalizers;
    while (true)
    {
        if (!do_process(item, normalizers) || !item.normalize)
            break;
    }
}


bool MediaProcess::do_process(const FileItem & item, std::vector<float> & normalizers) const
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
    const int srcChannelLayout = audioCodecIn->channel_layout;

    auto imageCodecIn = createCodec(imageStream ? imageStream->codecpar : nullptr);

    // *** Set up the input format ctx ***

    auto filters = filterFab_.create();


    int filterSampleRate = audioCodecIn->sample_rate;


    // SwrContext contains the audio file's sound quality parameters, such as the audio channel left/right flags (mono/stereo), 
    //  sampling format (e.g., 16 bits), and sampling rate (e.g., 44kHz)
    scoped_ptr<SwrContext> swr_in(nullptr, [] (SwrContext * d) { swr_free(&d); });
    {
        for (auto & filter : filters)
        {
            filterSampleRate = filter->agreeSamplerate(filterSampleRate);
        }

        for (auto & filter : filters)
        {
            filter->setSamplerate(filterSampleRate);
        }

        /* Force the input audio channel to have front left & right */
        /* Force the sampling format to be signed-16-bit-planar */
        /* Force the sampling rate to be 32k frames-per-sec (kHz), 44fps, or 48fps */
        if (audioCodecIn->channel_layout != (AV_CH_FRONT_LEFT | AV_CH_FRONT_RIGHT)
            || audioCodecIn->sample_fmt != AV_SAMPLE_FMT_S16P
            || (audioCodecIn->sample_rate != filterSampleRate))
        {   // Arguments: (swr_ctx, out, out, out, in, in, in, log_offset, log_ctx)
            swr_in.reset(swr_alloc_set_opts(NULL,
                                            AV_CH_FRONT_LEFT | AV_CH_FRONT_RIGHT, AV_SAMPLE_FMT_S16P, filterSampleRate,
                                            audioCodecIn->channel_layout, audioCodecIn->sample_fmt, audioCodecIn->sample_rate,
                                            0, NULL));
            if (!swr_in) throw MPError("swr_alloc_set_opts failed");

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
    params.channels = 2;
    params.channel_layout = AV_CH_FRONT_LEFT | AV_CH_FRONT_RIGHT;
    params.sample_rate = filterSampleRate;
    params.bit_rate = audioCodecIn->bit_rate;
    params.codec_id = avfmt_out->oformat->audio_codec;
    params.codec_type = AVMEDIA_TYPE_AUDIO;
    params.format = -1;

    // returns AVCodecContext*
    auto audioCodecOut = createCodec(&params, true);

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

    // If the output codec parameter is not what we want (s16p), then create awr_out as a format converter
    if (audioCodecOut->sample_fmt != AV_SAMPLE_FMT_S16P)
    {
        swr_out.reset(swr_alloc_set_opts(NULL,
                                         AV_CH_FRONT_LEFT | AV_CH_FRONT_RIGHT, audioCodecOut->sample_fmt, filterSampleRate,
                                         AV_CH_FRONT_LEFT | AV_CH_FRONT_RIGHT, AV_SAMPLE_FMT_S16P, filterSampleRate,
                                         0, NULL));
        if (!swr_out) throw MPError("swr_alloc_set_opts failed");
        if ((r = swr_init(swr_out)) != 0) throw MPError("input converter init failed", r);
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

    scoped_ptr<AVAudioFifo> fifo(av_audio_fifo_alloc(audioCodecOut->sample_fmt, audioCodecOut->channels, std::max(audioCodecOut->frame_size, 1)),
                                 [] (auto * d) { av_audio_fifo_free(d); });
    if (!fifo) throw MPError("failed to allocate fifo");

    std::vector<int16_t> lb, rb;
    std::vector<int16_t> lbOut, rbOut;

    int pts = 0;
    auto Pts = [&pts] (int samples)
    {
        auto r = pts;
        pts += samples;
        return r;
    };

    for (int i = 0; i < normalizers.size(); i++)
        filters[i]->normalizer = normalizers[i];

    //bool isFirstChunk = true;

    // Read one audio frame from the input file into a temporary packet.
    bool input_eof = false;
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
                        else if (r != AVERROR(EAGAIN))
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
        if (r != AVERROR_EOF && frame_in->nb_samples > 0)
        {   // If the current channel's layout id is different than the one specified in the input codec
            if (frame_in->channel_layout != srcChannelLayout)
            {
                // Oh, the channel layout can dynamically change in the middle lol
                throw MPError("channel layout had changed in the middle");
            }

            // Our filter expects the input to be signed a stereo 16-bit planar layout, so our data type is int16_t 
            int16_t * lbIn;
            int16_t * rbIn;

            // This is the input music file's decoded raw frame
            int nb_samples = frame_in->nb_samples;

            // If the input music file is NOT in the format we want (44kHz stereo, signe 16 bits, planar)
            if (swr_in)
            {
                // frame -> l/rb

                // An upper bound on the number of samples that the next swr_convert will output
                auto swrOutSamples = swr_get_out_samples(swr_in, nb_samples);

                // Dynamically increase the lb, rb vector size to be the maximum number of samples 
                //  across all decoded raw frames of the input music file
                if (lb.size() < swrOutSamples) lb.resize(swrOutSamples);
                if (rb.size() < swrOutSamples) rb.resize(swrOutSamples);

                uint8_t * buffers[] = { (uint8_t *)lb.data(), (uint8_t *)rb.data() };

                // Convert the input music file's decoded raw frame to be our desired format (signed 16-bits, planar, 44100 fps)
                nb_samples = swr_convert(swr_in, buffers, swrOutSamples, (const uint8_t **)frame_in->data, nb_samples);
                // cannot // assert(nb_samples == cwrOutSamples);

                // 128 bytes gap of Q2 ClaStr filter output to CH
                //if (isFirstChunk)
                //{
                //    nb_samples += 128;
                //    lb.insert(lb.begin(), 128, 0);
                //    rb.insert(rb.begin(), 128, 0);
                //    isFirstChunk = false;
                //}


                lbIn = lb.data();
                rbIn = rb.data();
            }
            // If the input music file is in the format we want
            else
            {
                lbIn = (int16_t *)frame_in->data[0];
                rbIn = (int16_t *)frame_in->data[1];

                // Prepare room for the filter
                if (lb.size() < nb_samples) lb.resize(nb_samples);
                if (rb.size() < nb_samples) rb.resize(nb_samples);
            }

            // Filter-processing

            // convert can produce no samples ... if the input is like 1 sample ... uhhh 
            if (nb_samples > 0)
            {
                // These are for output data (after filter-processing)
                if (lbOut.size() < nb_samples) lbOut.resize(nb_samples);
                if (rbOut.size() < nb_samples) rbOut.resize(nb_samples);

                for (auto filter = filters.begin(); filter != filters.end(); ++filter)
                {
                    (*filter)->filter(lbIn, rbIn, lbOut.data(), rbOut.data(), nb_samples);

                    // Reuse the same lb, rb, lbOut, rbOut buffers when processing through multiple filters
                    if (filter + 1 != filters.end())
                    {
                        lb.swap(lbOut);
                        rb.swap(rbOut);
                        lbIn = lb.data();
                        rbIn = rb.data();
                    }
                }

                // An extra processing to make the output frame to be our desired 
                //  channel layout, sampling rate, sample format (44kHz/48kHz/32kHz stereo s16p)
                if (swr_out)
                {
                    // l/rbOut -> fifo buffers

                    auto swrOutSamples = swr_get_out_samples(swr_out, nb_samples);
                    auto channels = av_get_channel_layout_nb_channels(audioCodecOut->channels);
                    auto bs = av_samples_get_buffer_size(0, channels, swrOutSamples, audioCodecOut->sample_fmt, 0);

                    // Resize to required size but send both buffers even for interleaved
                    if (lb.size() < bs) lb.resize(bs);
                    if (rb.size() < bs) rb.resize(bs);

                    uint8_t * buffersIn[] = { (uint8_t *)lbOut.data(), (uint8_t *)rbOut.data() };
                    uint8_t * buffersOut[] = { (uint8_t *)lb.data(), (uint8_t *)rb.data() };

                    nb_samples = swr_convert(swr_out, buffersOut, bs, (const uint8_t **)buffersIn, nb_samples);
                    writeToFifo(fifo, (void **)buffersOut, nb_samples);
                }
                // If the output is already stereo S16P 32kHz/44kHz/48kHz
                else
                {
                    uint8_t * buffers[] = { (uint8_t *)lbOut.data(), (uint8_t *)rbOut.data() };
                    writeToFifo(fifo, (void **)buffers, nb_samples);
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
            frame_out->channel_layout = audioCodecOut->channel_layout;
            frame_out->channels = audioCodecOut->channels;
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

    bool ripped = false;
    if (r == 0)
    {
        // Encode all unencoded remaining data and flush them to the output music file
        do {} while (encodeFrame(NULL, avfmt_out, audioCodecOut, audioStreamOutIndex) == 0);

        // Flush all encoded remaining data to the output music file
        if ((r = av_write_trailer(avfmt_out)) < 0)
            throw MPError("failed to write output the file trailer", r);

        audioCodecOut.reset();
        avfmt_out.reset();

        int i = 0;
        bool is_initial = !normalizers.size();
        for (auto filter = filters.begin(); normalize && filter != filters.end(); ++filter)
        {
            float factor_max = 1.0f, factor_min = 1.0f;

            if (is_initial)
                normalizers.push_back((*filter)->normalizer);

            // normalize only the first ripping filter 
            if (ripped)
            {
                continue;
            }
            // compute this processing round's max & min normalization factor
            if ((*filter)->global_max != 0x7FFF)
            {
                factor_max = (float)((*filter)->global_max) / (float)0x7FFF;
            }
            if ((*filter)->global_min != -0x8000)
            {
                factor_min = (float)((*filter)->global_min) / (float)(-0x8000);
            }
            // compute (or update) the normalization factor  
            if (factor_max != 1.0f || factor_min != 1.0f)
            {
                ripped = true;
                normalizers[i] *= std::max(factor_max, factor_min);

                msg() << "- Normalizing Filter[" << i << "]: Division Factor = " << normalizers[i];
            }
            i++;
        }
        // save the music file only if there is no ripping sound
        if (!normalize || !ripped)
            std::filesystem::rename(tempOut, item.output);
    }
    return r == 0 && ripped;
}
