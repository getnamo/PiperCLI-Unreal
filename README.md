# Piper CLI - Unreal

Experimental Piper plugin for Unreal via auto CLI wrapper.

## How to use

1. Clone and download and extract `PiperWin64-2023.11.14-2.7z` [download link](https://github.com/getnamo/PiperCLI-Unreal/releases/download/v0.1.external/PiperWin64-2023.11.14-2.7z) into `ThirdParty/Piper/` in your `PiperCli-Unreal` plugin root folder.

2. Download voice model from https://huggingface.co/rhasspy/piper-voices/tree/v1.0.0 e.g. for https://huggingface.co/rhasspy/piper-voices/tree/v1.0.0/en/en_US/joe/medium place `en_US-joe-medium.onnx` and `en_US-joe-medium.onnx.json` in your `PiperCLI-Unreal/ThirdParty/Piper/Win64/model/` folder.

3. Add `Piper` component to your actor of choice. Edit `PiperCLIParams` `Params` field to something like `--model ./model/en_US-joe-medium.onnx --output-raw`

4. Subscribe to `OnOutputBytes(Piper)` and convert your sound data from PCM to WAV to Unreal SoundWave using e.g. [SocketIOClient Plugin's](https://github.com/getnamo/SocketIOClient-Unreal/tree/master) core utility functions https://github.com/getnamo/SocketIOClient-Unreal/blob/master/Source/CoreUtility/Public/CUBlueprintLibrary.h#L90.

5. Play SoundWave.
