#include <iostream>
#include <libavcodec/avcodec.h>
#include <set>
#include <map>
#include <memory>
#include <functional>

#include "../wrapper/av.h"
#include "../wrapper/ffmpeg.h"
#include "../wrapper/codec.h"
#include "../wrapper/packet.h"
#include "../wrapper/videorescaler.h"
#include "../wrapper/audioresampler.h"
#include "../wrapper/avutils.h"

// API2
#include "../wrapper/format.h"
#include "../wrapper/formatcontext.h"
#include "../wrapper/codec.h"
#include "../wrapper/codeccontext.h"

using namespace std;
using namespace av;

int main()
{
    
    av::init();
    av::setFFmpegLoggingLevel(AV_LOG_DEBUG);
    av::Codec c = av::findDecodingCodec(AV_CODEC_ID_ILBC);
    cout << c.longName() << std::endl;
     
    return 0;
}
int main1(int argc, char **argv)
{
    if (argc < 3)
        return 1;

    av::init();
    av::setFFmpegLoggingLevel(AV_LOG_DEBUG);

    string uri {argv[1]};
    string out {argv[2]};

    error_code ec;

    //
    // INPUT
    //
    FormatContext ictx;
    ssize_t      videoStream = -1;
    VideoDecoderContext vdec;
    Stream      vst;

    int count = 0;

    ictx.openInput(uri, ec);
    if (ec) {
        cerr << "Can't open input\n";
        return 1;
    }

    ictx.findStreamInfo();

    for (size_t i = 0; i < ictx.streamsCount(); ++i) {
        auto st = ictx.stream(i);
        cout << "#### stream id:" << st.id() 
            << " index:" << st.index() 
            << " is audio:" << st.isAudio() 
            << " mediaType:" << st.mediaType() 
            << " spect ratio:"<< st.sampleAspectRatio().getDouble()
            << " fps:" << st.frameRate().getDouble() << '\n';
        if (st.mediaType() == AVMEDIA_TYPE_VIDEO) {
            videoStream = i;
            vst = st;
            break;
        }
    }

    if (vst.isNull()) {
        cerr << "Video stream not found\n";
        return 1;
    }

    if (vst.isValid()) {
        vdec = VideoDecoderContext(vst);
        vdec.setRefCountedFrames(true);

        vdec.open(Codec(), ec);
        if (ec) {
            cerr << "Can't open codec\n";
            return 1;
        }
    }


    //
    // OUTPUT
    //
    OutputFormat  ofrmt;
    FormatContext octx;

    //ofrmt.setFormat(string(), out);
    ofrmt.setFormat("","","video/MP2T");
    octx.setFormat(ofrmt);

    Codec       ocodec  = findEncodingCodec(ofrmt);
    cout << "##### Encode codec: " << ocodec.longName() << '\n';
    Stream      ost     = octx.addStream(ocodec);
    VideoEncoderContext encoder {ost};

    // Settings
    encoder.setWidth(vdec.width());
    encoder.setHeight(vdec.height());
    if (vdec.pixelFormat() > -1)
        encoder.setPixelFormat(vdec.pixelFormat());
    encoder.setTimeBase(Rational{1, 1000});


    encoder.setBitRate(vdec.bitRate());
   /* 
    ost.setFrameRate(vst.frameRate());
    

    octx.openOutput(out, ec);
    if (ec) {
        cerr << "Can't open output\n";
        return 1;
    }
*/
    encoder.open(Codec(), ec);
    if (ec) {
        cerr << "Can't opent encodec\n";
        return 1;
    }

    octx.dump();
    octx.writeHeader();
    octx.flush();

    return 0;
    //
    // PROCESS
    //
    while (true) {

        // READING
        Packet pkt = ictx.readPacket(ec);
        if (ec) {
            clog << "Packet reading error: " << ec << ", " << ec.message() << endl;
            break;
        }

        bool flushDecoder = false;
        // !EOF
        if (pkt) {
            if (pkt.streamIndex() != videoStream) {
                continue;
            }

            clog << "Read packet: pts=" << pkt.pts() << ", dts=" << pkt.dts() << " / " << pkt.pts().seconds() << " / " << pkt.timeBase() << " / st: " << pkt.streamIndex() << endl;
        } else {
            flushDecoder = true;
        }

        do {
            // DECODING
            auto frame = vdec.decode(pkt, ec);

            count++;
            //if (count > 200)
            //    break;

            bool flushEncoder = false;

            if (ec) {
                cerr << "Decoding error: " << ec << endl;
                return 1;
            } else if (!frame) {
                //cerr << "Empty frame\n";
                //flushDecoder = false;
                //continue;

                if (flushDecoder) {
                    flushEncoder = true;
                }
            }

            if (frame) {
                //clog << "Frame: pts=" << frame.pts() << " / " << frame.pts().seconds() << " / " << frame.timeBase() << ", " << frame.width() << "x" << frame.height() << ", size=" << frame.size() << ", ref=" << frame.isReferenced() << ":" << frame.refCount() << " / type: " << frame.pictureType()  << endl;

                // Change timebase
                frame.setTimeBase(encoder.timeBase());
                frame.setStreamIndex(0);
                frame.setPictureType();

                //clog << "Frame: pts=" << frame.pts() << " / " << frame.pts().seconds() << " / " << frame.timeBase() << ", " << frame.width() << "x" << frame.height() << ", size=" << frame.size() << ", ref=" << frame.isReferenced() << ":" << frame.refCount() << " / type: " << frame.pictureType()  << endl;
            }

            if (frame || flushEncoder) {
                do {
                    // Encode
                    Packet opkt = frame ? encoder.encode(frame, ec) : encoder.encode(ec);
                    if (ec) {
                        cerr << "Encoding error: " << ec << endl;
                        return 1;
                    } else if (!opkt) {
                        //cerr << "Empty packet\n";
                        //continue;
                        break;
                    }

                    // Only one output stream
                    opkt.setStreamIndex(0);

                    //clog << "Write packet: pts=" << opkt.pts() << ", dts=" << opkt.dts() << " / " << opkt.pts().seconds() << " / " << opkt.timeBase() << " / st: " << opkt.streamIndex() << endl;

                    octx.writePacket(opkt, ec);
                    if (ec) {
                        cerr << "Error write packet: " << ec << ", " << ec.message() << endl;
                        return 1;
                    }
                } while (flushEncoder);
            }

            if (flushEncoder)
                break;

        } while (flushDecoder);

        if (flushDecoder)
            break;
    }

    octx.writeTrailer();
    return 0;
}
