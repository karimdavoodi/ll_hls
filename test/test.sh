ffmpeg -thread_queue_size 32 -v error \
        -f v4l2 -video_size 640x360 -framerate 30 -i /dev/video0 \
        -c:v libx264 -b:v 1m -preset fast -pix_fmt yuv420p -tune zerolatency -x264-params keyint=5:min-keyint=5 -r 30\
        -c:a aac -ac 2 -b:a 96k \
        -f ssegment -segment_list_flags live \
            -segment_list_type csv -segment_time 0.3 -segment_list pipe:1 \
            -segment_wrap 99 work/out%02d.ts | cat
