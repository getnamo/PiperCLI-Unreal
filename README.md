# Piper CLI - Unreal

[Piper TTS](https://github.com/rhasspy/piper) plugin for Unreal via [simple CLI wrapper](https://github.com/getnamo/PiperCLI-Unreal/blob/main/Source/CLISystem/Public/SubProcess.h#L68).




## How to use

1. Grab latest [release](https://github.com/getnamo/PiperCLI-Unreal/releases) and extract plugins folder into your project root folder.

2. Download and extract `PiperWin64-2023.11.14-2.7z` [link](https://github.com/getnamo/PiperCLI-Unreal/releases/download/v0.1.external/PiperWin64-2023.11.14-2.7z) into `ThirdParty/Piper/` in your `PiperCli-Unreal` plugin root folder.
 
3. Download voice model from https://huggingface.co/rhasspy/piper-voices/tree/v1.0.0 e.g. for https://huggingface.co/rhasspy/piper-voices/tree/v1.0.0/en/en_US/joe/medium place `en_US-joe-medium.onnx` and `en_US-joe-medium.onnx.json` in your `PiperCLI-Unreal/ThirdParty/Piper/Win64/model/` folder.

4. Start your unreal project
  
3. Add `Piper` component to your actor of choice. Edit `PiperParams` model `model/en_US-joe-medium.onnx`

v0.3

4. Subscribe to `OnOutputBytes(Piper)` and convert your sound data from PCM to WAV to Unreal SoundWave using e.g. [SocketIOClient Plugin's](https://github.com/getnamo/SocketIOClient-Unreal/tree/master) core utility functions https://github.com/getnamo/SocketIOClient-Unreal/blob/master/Source/CoreUtility/Public/CUBlueprintLibrary.h#L90.

v0.4

4. Subscribe to `OnAudioGenerated` which output `USoundWave*` outputs instead of PCM bytes.

5. Play SoundWave. Optionally use `USoundWaveQueueComponent` to queue up all the soundwaves being generated faster than real-time and play them back at target world location in sequence.
