# Steam Audio and HRTF data

Steam Audio 4.8.1, Copyright Valve Corporation, is distributed under Apache-2.0.
The original license and the SDK's third-party notices accompany the runtime.

Official SDK: https://github.com/ValveSoftware/steam-audio/releases/tag/v4.8.1
SDK ZIP SHA-256: `4a0aa5ec1176f38f0b0993a37c2259d9e86f27e22d5e24f83ec4c3cb9a1d5449`.

The CIPIC subject 124 SOFA data is the source of Valve's default HRTF. The
SOFA reader converts its impulse responses to the game's 32 kHz sample rate
at startup. This avoids resampling the game's original output.

Source: https://github.com/ValveSoftware/steam-audio/blob/v4.8.1/core/data/hrtf/cipic_124.sofa
SHA-256: `c28ff4a874ac889ec0c5885ca524762a70d56984232ff7aadcd9c15d32e1cfb6`.

Copyright (c) 2001 The Regents of the University of California. All Rights Reserved.
See the CIPIC HRTF Database section of THIRDPARTY.md for its complete notice
and permission to reproduce and use the data.
