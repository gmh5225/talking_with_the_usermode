# talking_with_the_usermode
Credits <br>
- [Lord of the rings](https://idov31.github.io/2022/07/14/lord-of-the-ring0-p1.html)
- [MSDN Hello World](https://learn.microsoft.com/en-us/windows-hardware/drivers/gettingstarted/writing-a-very-small-kmdf--driver)

Example on how a Windows Driver and a usermode application can communicate. <br>
Currently the Driver is able to take a IOCTL Request from the usermode application and protect a given pid from being terminated. <br>
This is done by stripping the handle to the protected process from its PROCESS_TERMINATE rights. When OpenProcess is called and a Handle to the protected program is created the Callback interferes.
