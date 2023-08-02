# sox ~/tmp/remorse-test.wav -t raw -e signed -b 16 -c 1 -r 48k - | ./remorse | sox -t raw -e signed -b 16 -c 1 -r 48k - ~/tmp/remorse-out.wav

ffmpeg -i before.mp4 -an -codec:v copy /tmp/video.mp4
ffmpeg -i before.mp4 -ar 48000 /tmp/audio.wav
sox /tmp/audio.wav -t raw -e signed -b 16 -c 1 -r 48k - | ./remorse | sox -t raw -e signed -b 16 -c 1 -r 48k - /tmp/audio2.wav
ffmpeg -i /tmp/audio2.wav -i /tmp/video.mp4 -ab 128k -ar 48000 -codec:a mp3 -codec:v copy after.mp4
rm -f /tmp/audio.wav /tmp/audio2.wav /tmp/video.mp4
