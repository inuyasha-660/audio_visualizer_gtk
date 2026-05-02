# Audio Visualizer
GTK4 实现的简单音频可视化工具

## 功能
- [x] 捕获桌面音频(PipeWire)
- [x] 读取音频文件
- [x] 波形图
- [x] 频谱图
- [x] 模拟示波器(X-Y)

## 支持的音频格式
[libsndfile: Supported formats](https://libsndfile.github.io/libsndfile/formats.html)

## 预览
| 波形图 | 频谱图 | X-Y |
|-------|-------|-----|
| ![wave](/resources/wave.png) | ![wave](/resources/fft.png) |![wave](/resources/x-y.png)

## 构建
依赖:
  * [GTK4](https://www.gtk.org/)
  * [Adw – 1](https://gnome.pages.gitlab.gnome.org/libadwaita/doc/main/index.html)
  * [PortAudio](https://www.portaudio.com/)
  * [Libsndfile](https://libsndfile.github.io/libsndfile/)
  * [FFTW](https://www.fftw.org/)
  * [PipeWire](https://pipewire.org/)

### Meson
``````sh
meson setup build
cd build
meson compile
``````
产物为 ``audio_visualizer``
