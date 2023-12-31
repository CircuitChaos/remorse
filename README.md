# Morse audio signal beautifier

This is a small tool made for my new YouTube channel called *Jokes in Morse code*: https://youtube.com/@MorseJokes.

This channel contains short jokes sent by me in Morse code, so I have some motivation to practice sending. Here's the channel trailer.

<a href="http://www.youtube.com/watch?feature=player_embedded&v=OJxj9cMLNwU" target="_blank"><img src="http://img.youtube.com/vi/OJxj9cMLNwU/0.jpg" alt="Video" width="240" height="180" border="10" /></a>

https://youtu.be/OJxj9cMLNwU

I connected my Morse code training generator to the phone, but this generator is very simple. It doesn't contain envelope shaping, 
for example. That's why I wrote this small C program to analyze audio from this generator and produce a clean, nice sine wave with 
a precisely defined frequency, amplitude, and envelope.

You can see the difference below. First recording is from the generator.

<a href="http://www.youtube.com/watch?feature=player_embedded&v=N5Lu3lELRRk" target="_blank"><img src="http://img.youtube.com/vi/N5Lu3lELRRk/0.jpg" alt="Video" width="240" height="180" border="10" /></a>

https://youtu.be/N5Lu3lELRRk

And the one below has been passed through **remorse**.

<a href="http://www.youtube.com/watch?feature=player_embedded&v=rSP13v_gkoE" target="_blank"><img src="http://img.youtube.com/vi/rSP13v_gkoE/0.jpg" alt="Video" width="240" height="180" border="10" /></a>

https://youtu.be/rSP13v_gkoE

## Usage

See *test.sh* file for example, and call `./remorse -h` to see command line options (or just read `help` function in *remorse.c*).

## TODO

* Check sanity of command-line supplied configuration
* Check for errors when writing output stream
* Add a CLI option to parametrize peak detector's return period (now it's fixed and dependent on the sampling frequency)
* Add a better envelope (currently I'm using a sine envelope, which is good enough, but probably a raised cosine would be better)
