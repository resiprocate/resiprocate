#
# Copyright (C) 2015 Daniel Pocock http://danielpocock.com
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
#

SUBDIRS = . data

EXTRA_DIST = playback_prompt.wav
EXTRA_DIST += record_prompt.wav
EXTRA_DIST += resiprocate.manager
EXTRA_DIST += resiprocate.service

#AM_CXXFLAGS = -DUSE_ARES
AM_CPPFLAGS = -fPIC $(QT5_CFLAGS) $(TP_QT5_CFLAGS)

bin_PROGRAMS = telepathy-resiprocate
telepathy_resiprocate_LDADD = ../../resip/recon/librecon.la
telepathy_resiprocate_LDADD += ../../reflow/libreflow.la
telepathy_resiprocate_LDADD += -lsipXsdp -lsipXmediaProcessing -lsipXmedia -lsipXport -lsipXtack
telepathy_resiprocate_LDADD += ../../resip/dum/libdum.la
telepathy_resiprocate_LDADD += ../../resip/stack/libresip.la
telepathy_resiprocate_LDADD += ../../rutil/librutil.la
telepathy_resiprocate_LDADD += $(TP_QT5_LIBS) $(QT5_LIBS)
telepathy_resiprocate_LDADD += @LIBSSL_LIBADD@ @LIBPTHREAD_LIBADD@

nodist_telepathy_resiprocate_SOURCES = moc_Connection.cxx \
	moc_TextChannel.cxx \
	moc_MyConversationManager.cxx \
	moc_MyUserAgent.cxx \
	moc_MyInstantMessage.cxx \
	moc_Protocol.cxx \
	moc_SipCallChannel.cxx

telepathy_resiprocate_SOURCES = Common.cxx \
	Connection.cxx \
	TextChannel.cxx \
	MyConversationManager.cxx \
	MyMessageDecorator.cxx \
	MyUserAgent.cxx \
	MyInstantMessage.cxx \
	Protocol.cxx \
	SipCallChannel.cxx \
	TelepathyConversationProfile.cxx \
	TelepathyMasterProfile.cxx \
	TelepathyParameters.cxx \
	telepathy-resiprocate.cxx

noinst_HEADERS = Common.hxx \
	Connection.hxx \
	TextChannel.hxx \
	MyConversationManager.hxx \
	MyMessageDecorator.hxx \
	MyUserAgent.hxx \
	MyInstantMessage.hxx \
	SipCallChannel.hxx \
	TelepathyConversationProfile.hxx \
	TelepathyMasterProfile.hxx \
	TelepathyParameters.hxx \
	Protocol.hxx

moc_%.cxx:	%.hxx
	QT_SELECT=qt5 moc $< -o $@

BUILT_SOURCES = playback_prompt.h record_prompt.h

playback_prompt.h:
	echo "#ifndef _playback_prompt_h_" > playback_prompt.h
	echo "#define _playback_prompt_h_" >> playback_prompt.h
	xxd -i -u playback_prompt.wav >> playback_prompt.h
	sed  -i 's/_wav//g' playback_prompt.h
	echo "#endif" >> playback_prompt.h

record_prompt.h:
	echo "#ifndef _record_prompt_h_" > record_prompt.h
	echo "#define _record_prompt_h_" >> record_prompt.h
	xxd -i -u record_prompt.wav >> record_prompt.h
	sed  -i 's/_wav//g' record_prompt.h
	echo "#endif" >> record_prompt.h


