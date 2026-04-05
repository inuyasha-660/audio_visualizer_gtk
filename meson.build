project('audio_visualizer_gtk', 'c')

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
  cc.find_library('mp3lame')
  ]

src = files(
  'main.c',
  'ui.c',
  'audio.c'
  )

executable('audio_visualizer', src, dependencies: dep)
