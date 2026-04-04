project('audio_visualizer_gtk', 'c')

dep = [dependency('gtk4'), dependency('libadwaita-1')]

src = files(
  'main.c',
  'ui.c'
  )

executable('audio_visualizer', src, dependencies: dep)
