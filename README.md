# User Activity Monitor
A lite project in c++ to monitor user activities (in Windows platform only)<br>
*Origirnal log message were completed in Simplified Chinese*

## ⚖️ License & Attribution
- **Open Source Requirement:** Any developer utilizing the code from this repository **MUST** release their derivative work as open-source, strictly in accordance with the [LICENSE](./LICENSE) file.
- **Copyright Notice:** The design copyrights for `elevator.ico` and `main_program.ico` belong exclusively to the **Archiv Team**.
<br>

## ⚖️ Third Party Licenses
This project utilizes **curl**.
<br>

## 🛠️ Build & Configuration Notes
- **Optional Resource Files:** The `*.rc` and `*.ico` files are not strictly required for development. If you do not need them, simply remove the `{FILENAME}.rc` entries from the `add_executable(...)` section in your `CMakeLists.txt`.

- **Digital Signature:** Digital signing is optional. If not needed, remove all `add_custom_command(...)` entries containing `signtool.exe` from `CMakeLists.txt`. If you choose to use it, you can place the signature files (`*.pvx`) anywhere you prefer, but remember to update the corresponding paths in `CMakeLists.txt`.
  
- **SMTP Feature:** The fields in `SMTPConfiguration.h` must be manually populated. Alternatively, if you do not need the SMTP feature, you can disable it by setting `SMTP_FEATURE_ENABLED` to `false` in `Generic_Configuration.h` *(Note: This alternative method is currently untested)*.
<br>

## Other Things
### Contributions Welcome: 
We warmly welcome developers to join this project and become part of our team! Whether you are fixing bugs, adding new features, or improving documentation, your contributions are highly valued.

### Community Vitality: 
We deeply care about the vitality of our community. We believe that an active and collaborative environment is the key to making this project better. Feel free to open issues, share your ideas, or start discussions!
<br>

## Developers:
### Director / Cheif Developer
Byakuya (aka Qi-An Chen, 2026/8/12)
<br>
<br>

## Donater / Corporator:
### Corportator
Nebula (For the all installtation tests on their server envrionment)<br>
*see also @cyhcyh*
