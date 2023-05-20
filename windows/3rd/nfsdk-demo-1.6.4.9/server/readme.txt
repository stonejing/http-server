Overview
=====================================

This is a demo version of NetFilter SDK 2.0 Gateway Filter. 

The demo driver filters no more than 1000000 TCP connections and UDP sockets. 
After exceeding this limit the filtering continues again after system reboot.

The driver API is provided as a DLL with C and C++ interfaces. 

Package contents
=====================================
bin\Release - x86 and x64 versions of APIs with C++ interface, pre-built samples and the driver registration utility.
bin\Release_c_api - x86 and x64 versions of APIs with C interface, pre-built samples and the driver registration utility.

bin\driver - the binaries of driver for x86 and x64 platforms.

samples - the examples of using APIs in C/C++/Deplhi/.NET
samples\CSharp - .NET API and C# samples.
samples\Delphi - Delphi API and samples.
Help - API documentation.


Driver installation
=====================================
Use the scripts bin\install_driver.bat and bin\install_driver_x64.bat for installing and registering the network hooking driver on x86 and x64 systems respectively. 
The driver starts immediately and reboot is not required.

Run bin\uninstall_driver.bat to remove the driver from system.

Elevated administrative rights must be activated explicitly on Vista and later for registering the driver (run the scripts using "Run as administrator" context menu item in Windows Explorer). 

For Windows Vista x64 and later versions of the Windows family of operating systems, kernel-mode software must have a digital signature to load on x64-based computer systems. 
The included demo versions of the network hooking driver are signed. But the drivers in Standard and Full sources versions are not signed.
For the end-user software you have to obtain the Code Signing certificate and sign the driver.


Supported platforms: 
    Windows 8/2008/2012/10 x86/x64
