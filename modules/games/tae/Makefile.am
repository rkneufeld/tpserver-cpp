rulesetlib_LTLIBRARIES = libtae.la

rulesetlibdir = $(libdir)/tpserver/ruleset

libtae_la_SOURCES = tae.cpp universe.cpp spaceobject.cpp emptyobject.cpp \
					starsystem.cpp planet.cpp ownedobject.cpp fleet.cpp \
					colonize.cpp taeturn.cpp move.cpp attack.cpp \
					fleetorder.cpp fleetbuilder.cpp reinforce.cpp

libtae_la_LDFLAGS = -module

noinst_HEADERS = tae.h universe.h spaceobject.h emptyobject.h starsystem.h \
				 planet.h ownedobject.h fleet.h colonize.h taeturn.h move.h \
				 attack.h fleetorder.h fleetbuilder.h reinforce.h

EXTRA_DIST = tpserver-cpp-tae.xml

tpconfigdir = $(datadir)/tp/servers
tpconfig_DATA = tpserver-cpp-tae.xml