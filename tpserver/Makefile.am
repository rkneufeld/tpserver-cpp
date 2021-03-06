## Process this file with automake to produce Makefile.in

bin_PROGRAMS = tpserver-cpp

if GNUTLS
GNUTLSSOURCES = tlsmanager.cpp tlssocket.cpp \
		playertlsconn.cpp httpssocket.cpp
GNUTLSHEADERS = tlsmanager.h tlssocket.h playertlsconn.h httpssocket.h 
else
GNUTLSSOURCES =
GNUTLSHEADERS =
endif

if AVAHI
AVAHISOURCES = avahi.cpp
AVAHIHEADERS = avahi.h
else
AVAHISOURCES =
AVAHIHEADERS =
endif

tpserver_cpp_SOURCES = $(GNUTLSSOURCES) main.cpp connection.cpp \
		frame.cpp game.cpp logging.cpp net.cpp settings.cpp player.cpp object.cpp \
		order.cpp \
		vector3d.cpp board.cpp message.cpp ordermanager.cpp objecttypemanager.cpp \
		playerconnection.cpp playertcpconn.cpp tcpsocket.cpp property.cpp \
		propertyvalue.cpp component.cpp design.cpp designstore.cpp tpscheme.cpp category.cpp \
		persistence.cpp objectmanager.cpp playermanager.cpp listensocket.cpp boardmanager.cpp \
		filelogger.cpp syslogger.cpp consolelogger.cpp resourcedescription.cpp \
		resourcemanager.cpp pluginmanager.cpp httpsocket.cpp playerhttpconn.cpp \
		admintcpsocket.cpp adminconnection.cpp admintcpconn.cpp adminlogger.cpp \
		command.cpp commandmanager.cpp \
		$(AVAHISOURCES) advertiser.cpp metaserverpublisher.cpp metaserverconnection.cpp \
		orderparameter.cpp spacecoordparam.cpp timeparameter.cpp objectorderparameter.cpp \
		stringparameter.cpp listparameter.cpp prng.cpp playeragent.cpp turnprocess.cpp \
		objectparameter.cpp objectparametergroup.cpp position3dobjectparam.cpp \
		velocity3dobjectparam.cpp sizeobjectparam.cpp integerobjectparam.cpp \
		referenceobjectparam.cpp refquantitylistobjectparam.cpp resourcelistobjectparam.cpp \
		orderqueueobjectparam.cpp orderqueue.cpp turntimer.cpp asynctimeremaining.cpp \
		playerview.cpp componentview.cpp designview.cpp objectview.cpp objecttype.cpp \
		objectparametergroupdesc.cpp objectbehaviour.cpp objectinfo.cpp objectrelationships.cpp
		
headersdir = $(includedir)/tpserver
headers_HEADERS = connection.h \
	frame.h game.h logging.h net.h settings.h \
	player.h object.h \
	order.h \
	vector3d.h board.h message.h ordermanager.h \
	objecttypemanager.h \
	playerconnection.h adminconnection.h \
	refsys.h property.h propertyvalue.h component.h \
	design.h designstore.h \
	ruleset.h tpscheme.h category.h persistence.h \
	objectmanager.h \
	playermanager.h listensocket.h boardmanager.h \
	resourcedescription.h resourcemanager.h settingscallback.h \
	orderparameter.h spacecoordparam.h timeparameter.h objectorderparameter.h stringparameter.h \
	listparameter.h timercallback.h prng.h playeragent.h turnprocess.h result.h \
	objectparameter.h objectparametergroup.h position3dobjectparam.h \
	velocity3dobjectparam.h sizeobjectparam.h integerobjectparam.h \
	referenceobjectparam.h refquantitylistobjectparam.h resourcelistobjectparam.h \
	orderqueueobjectparam.h orderqueue.h turntimer.h playerview.h componentview.h \
	designview.h objectview.h objecttype.h objectparametergroupdesc.h \
	objectbehaviour.h objectinfo.h objectrelationships.h protocol.h \
	command.h commandmanager.h

noinst_HEADERS = playertcpconn.h tcpsocket.h filelogger.h syslogger.h \
	consolelogger.h logsink.h $(GNUTLSHEADERS) pluginmanager.h httpsocket.h \
	playerhttpconn.h $(AVAHIHEADERS) advertiser.h publisher.h metaserverpublisher.h \
	admintcpsocket.h admintcpconn.h adminlogger.h \
	metaserverconnection.h asyncframe.h asynctimeremaining.h

INCLUDES = $(LTDLINCL)

# FIXME: the dlopens should be dependent on what is configured to run.
tpserver_cpp_LDADD = @libgnutls_LIBS@ @avahiclient_LIBS@ @BOOST_LDFLAGS@ @BOOST_SIGNALS_LIB@ $(LIBLTDL) "-dlopen" self
tpserver_cpp_DEPENDENCIES = $(LIBLTDL) 
AM_CXXFLAGS = @libgnutls_CFLAGS@ @avahiclient_CFLAGS@
tpserver_cpp_LDFLAGS = -export-dynamic 


