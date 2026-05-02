project('audio_visualizer_gtk', 'c',
    default_options: [ 
        'buildtype=release'
  ],)

cc = meson.get_compiler('c')

dep = [
  dependency('gtk4'),
  dependency('libadwaita-1'),
  dependency('portaudio-2.0'),
  dependency('sndfile'),
  dependency('fftw3f'),
  dependency('opus'),
  dependency('flac'),
  dependency('ogg'),
  dependency('libmpg123'),
  dependency('vorbis'),
  dependency('vorbisenc'),
  dependency('libpipewire-0.3'),
  cc.find_library('mp3lame'),
  ]

src = files(
  'main.c',
  'ui.c',
  'audio.c'
  )

executable('audio_visualizer', src, dependencies: dep)
