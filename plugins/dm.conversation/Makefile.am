AM_CPPFLAGS = -I$(top_srcdir)/include -I$(top_srcdir)/libs \
               $(XML_CFLAGS)

pluginsdir = $(pkglibdir)/plugins
plugins_LTLIBRARIES = dm_conversation.la

dm_conversation_la_LIBADD = $(top_builddir)/libs/wxutil/libwxutil.la \
                            $(top_builddir)/libs/xmlutil/libxmlutil.la
dm_conversation_la_LDFLAGS = -module -avoid-version \
                             $(WX_LIBS) $(XML_LIBS)
dm_conversation_la_SOURCES = plugin.cpp \
                      ConversationKeyExtractor.cpp \
                      ConversationCommandInfo.cpp \
                      CommandArgumentItem.cpp \
                      ConversationEntity.cpp \
                      CommandEditor.cpp \
                      ConversationEditor.cpp \
                      ConversationCommand.cpp \
                      ConversationCommandLibrary.cpp \
                      ConversationDialog.cpp
