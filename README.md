# REFramework Upscaler — macOS / CrossOver

A modified build of [REFramework](https://github.com/praydog/REFramework) focused on bringing upscaling support to Resident Evil games running through **CrossOver / Wine on macOS**.

This project includes two builds:

*  **Standard REFramework** — a macOS/CrossOver-compatible REFramework build for supported Resident Evil games.
*  **Upscaler Build** — includes the upscaler functionality, allowing supported games to use modern upscaling features such as **FSR 3**.


## Compatibility

Originally developed and tested with **Resident Evil 4 Remake**, but the builds can also work with other Resident Evil games that are compatible with REFramework.

### 💻 Tested System

|                         |              |
| ----------------------- | ------------ |
| **Mac**                 | Mac mini     |
| **Chip**                | Apple M2 Pro |
| **RAM**                 | 16 GB        |
| **OS**                  | macOS 26.3   |
| **Compatibility Layer** | CrossOver 26 |


## Standard REFramework

Tested working with **Resident Evil 2 Remake** on macOS through CrossOver.

<img width="2560" height="1440" alt="Screenshot 2026-09-04 at 2 49 56 PM" src="https://github.com/user-attachments/assets/1390efed-1d86-4e7a-8383-41dd8211f581" />

<img width="2560" height="1440" alt="Screenshot 2026-09-04 at 2 49 29 PM" src="https://github.com/user-attachments/assets/aa41c7c4-ffec-475c-a86e-671d795ea1d6" />


## Upscaler Build

Tested working with **Resident Evil 4 Remake**, demonstrating the upscaler functionality running through CrossOver on macOS.

<img width="2560" height="1440" alt="Screenshot 2026-09-04 at 3 06 43 PM" src="https://github.com/user-attachments/assets/85bc1e92-aa23-42e8-8a82-af2bd3bee5c4" />


## ⚠️ Known Limitation

For an unknown reason, **FSR 3 does not currently appear in the upscaler selection** when using this build on macOS/CrossOver. The available options currently include **DLSS** and **XeSS**, but FSR 3 is not displayed.

This may be related to the CrossOver/Wine environment or the current implementation and may require further investigation.

* ### Recommended Mod

For **Resident Evil 4 Remake**, I also recommend using the **[RE4R mod from Nexus Mods](https://www.nexusmods.com/residentevil42023/mods/4195?tab=files&file_id=17839)** alongside this build. It can be useful for improving the overall upscaling experience and is worth trying if you're running RE4R through CrossOver.

## Open Source / macOS Patch

This repository includes the **`REFramework-GPTK-applied.patch`** patch containing the modifications used to adapt REFramework for **macOS / CrossOver / GPTK**.

The patch is provided so developers can **inspect, modify, experiment with, and rebuild** the implementation themselves. It can also serve as a starting point for further improvements to macOS compatibility and the upscaler functionality.

* ### Building & Development

The patch is intended for developers who want to build their own version of the project or contribute improvements.

**Patch:** `REFramework-GPTK-applied.patch`

If you make improvements or fixes, feel free to share them with the community so the macOS implementation can continue to improve.


## Credits

This project builds upon the work of the **REFramework** project by **[praydog](https://github.com/praydog/REFramework)**.

* **[praydog](https://github.com/praydog/REFramework)** — Original REFramework framework, mod loader, and scripting platform.
* **[Realyxl](https://github.com/realyxl/REFramework)** — Credit for the code and work used as the basis for adapting the newer REFramework/upscaler functionality for **CrossOver/GPTK**.
* **Nim-9 "Me"** — CrossOver/GPTK compatibility work, modifications, testing, debugging, and the **newer upscaler port** included in this release.

Full credit goes to the original authors and contributors whose work made this project possible.


## ‼️ Disclaimer

This is an unofficial modification of REFramework and is not affiliated with or endorsed by CAPCOM or the original REFramework developers.

For installation instructions and downloads, see the **Releases** section.

