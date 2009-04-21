# $Id: Makefile,v 1.9 2004/05/10 01:12:46 jason Exp $

BUILD = ../build
include $(BUILD)/Makefile.pre

CXXFLAGS += -I.. -I./s2c

PACKAGES += RUTIL OPENSSL PTHREAD S2C

CODE_SUBDIRS := 

TARGET_LIBRARY := libp2p

TESTPROGRAMS := ParsingTest.cxx 

SRC 	+= \
	ChordTopology.cxx \
	ChordUpdate.cxx \
	Connect.cxx \
	ConnectBase.cxx \
	DestinationId.cxx \
	Dispatcher.cxx \
	Find.cxx \
	FlowId.cxx \
	ForwardingLayer.cxx \
	Join.cxx \
	Leave.cxx \
	Message.cxx \
	MessageHelper.cxx \
	MessageStructsGen.cxx \
	NodeId.cxx \
	P2PStack.cxx \
	P2PSubsystem.cxx \
	ResourceId.cxx \
	SelectTransporter.cxx \
	Signable.cxx \
	SignatureContext.cxx \
	TestMake.cxx \
	TopologyAPI.cxx \
	Transporter.cxx \
	TransporterMessage.cxx \
	Update.cxx \
	BatchMessages.cxx \

include $(BUILD)/Makefile.post

#======================================================================
#  Copyright (c) 2008, Various contributors to the Resiprocate project
#  All rights reserved.
#  
#  Redistribution and use in source and binary forms, with or without
#  modification, are permitted provided that the following conditions
#  are met:
#
#      - Redistributions of source code must retain the above copyright
#        notice, this list of conditions and the following disclaimer.
#
#      - Redistributions in binary form must reproduce the above
#        copyright notice, this list of conditions and the following
#        disclaimer in the documentation and/or other materials
#        provided with the distribution.
#
#      - The names of the project's contributors may not be used to
#        endorse or promote products derived from this software without
#        specific prior written permission.
#
#  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
#  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
#  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
#  FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
#  COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
#  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
#  BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
#  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
#  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
#  LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
#  IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
#  THE POSSIBILITY OF SUCH DAMAGE.
#======================================================================
