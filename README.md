# talking_with_the_usermode
Credits <br>
- [Lord of the rings](https://idov31.github.io/2022/07/14/lord-of-the-ring0-p1.html)
- [MSDN Hello World](https://learn.microsoft.com/en-us/windows-hardware/drivers/gettingstarted/writing-a-very-small-kmdf--driver)

Example on how a Windows Driver and a usermode application can communicate. <br>
Currently the Driver is able to take a IOCTL Request from the usermode application and protect a given pid from being terminated. <br>
This is done by stripping the handle to the protected process from its PROCESS_TERMINATE rights. When OpenProcess is called and a Handle to the protected program is created the Callback interferes.



####Important CLI commands mentioned in [Lord of the rings](https://idov31.github.io/2022/07/14/lord-of-the-ring0-p1.html) to start the driver.

##### Driver Testing

To test it in your testing environment run those commands with elevated cmd:

```cmd
bcdedit /set testsigning on
```

After rebooting, create a service and run the driver:

```cmd
sc create nidhogg type= kernel binPath= C:\Path\To\Driver\Nidhogg.sys
sc start nidhogg
```

##### Debugging

To debug the driver in your testing environment run this command with elevated cmd and reboot your computer:

```cmd
bcdedit /debug on
```

After the reboot, you can see the debugging messages in tools such as
