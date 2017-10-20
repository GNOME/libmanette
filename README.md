# libmanette

libmanette is a small GObject library giving you simple access to game
controllers.

This library is intended for software needing a painless access to game
controllers from any programming language and with little dependencies.

It supports the de-facto standard gamepads as defined by the
[W3C standard Gamepad specification](https://www.w3.org/TR/gamepad/) or as
implemented by the
[SDL GameController](https://wiki.libsdl.org/CategoryGameController). More game
controller kinds could be supported in the future if needed. Mapping of the
devices is handled transparently and internally by the library using the popular
SDL mapping string format.

The API is inspired by the device and event handling of GDK, so anybody used to
GTK+ should feel right at home.

## Building and Installing the Library

Meson is the buildsystem of libmanette, to build and install it run:

	# meson build
	# cd build
	# ninja
	# ninja install

## Testing the Library

libmanette comes with the manette-test tool, which will display events
from game controllers. You can run it from the build directory with:

	# demos/manette-test/manette-test
