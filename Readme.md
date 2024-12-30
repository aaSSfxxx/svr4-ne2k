# NE2000 driver for UNIX System V Release 4.0

## Summary

This is an attempt to develop a driver for the NE2000-like ISA cards, which
are not supported "out-of-the-box" by old SVR4. I started this project in
order to get in touch with driver development, and because it's fun to try
to make old dinosaur UNIXes great again !

For the moment, the driver can't be used with the SVR network stack, even if it can forward received packets into userland. This driver is developed on SVR Release 4.0 Version 3.0.

## TODO
- Implement the STREAMS DLPI interfaces so SVR network stack can make use of the driver
- Implement packet sending
- Broadcast incoming packet into all clone devices
- XXX
- Testing
- In future, far, far away, handle the PCI-variant NE2000

## License

This code is licensed under the WTFPL.