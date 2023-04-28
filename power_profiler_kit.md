

# Power Profiler Kit II

[Documentation](https://www.nordicsemi.com/Products/Development-hardware/Power-Profiler-Kit-2/GetStarted)

## Ubuntu install

[Linux installation file](https://www.nordicsemi.com/Products/Development-tools/nRF-Connect-for-Desktop/Download#infotabs)


```bash
cd ~/Downloads

chmod +x <file_name>.AppImage

# Dependencies for Ubuntu 22.04
sudo apt install libfuse2

# Execute program
./<file_name>.AppImage
```

### Unknown device problems
Power Profiler v3.3.5 can detect a device but can´t recognize it. 

Download `nrf-udev_1.0.1-all.deb` from: [/dev/ttyACM*](https://github.com/NordicSemiconductor/nrf-udev/releases)

```bash
sudo chmod +x nrf-udev_1.0.1-all.deb 

sudo dpkg -i nrf-udev_1.0.1-all.deb

reboot
```



