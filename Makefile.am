EXTRA_DIST = GITREVISION
SUBDIRS = include src Socket test

pkgconfigdir = $(datadir)/pkgconfig
pkgconfig_DATA = @PACKAGE_NAME@-@PACKAGE_VERSION_MAJOR@.@PACKAGE_VERSION_MINOR@.pc
@PACKAGE_NAME@-@PACKAGE_VERSION_MAJOR@.@PACKAGE_VERSION_MINOR@.pc: @PACKAGE_NAME@.pc
	cp "$<" "$@"
