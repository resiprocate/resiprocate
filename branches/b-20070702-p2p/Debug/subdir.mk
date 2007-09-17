################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../AsyncStage.cpp \
../ChordDhtStage.cpp \
../EventInfo.cpp \
../EventState.cpp \
../ForwardingStage.cpp \
../GenericDhtStage.cpp \
../GenericStage.cpp \
../InputStage.cpp \
../Locus.cpp \
../NetworkTransportStage.cpp \
../RoutingStage.cpp \
../StorageStage.cpp \
../TcpTransportStage.cpp \
../UdpTransportStage.cpp \
../Util.cpp \
../main.cpp 

CC_SRCS += \
../CommandBlock.cc \
../DataBlock.cc \
../FdSet.cc \
../ForwardBlock.cc \
../Socket.cc 

OBJS += \
./AsyncStage.o \
./ChordDhtStage.o \
./CommandBlock.o \
./DataBlock.o \
./EventInfo.o \
./EventState.o \
./FdSet.o \
./ForwardBlock.o \
./ForwardingStage.o \
./GenericDhtStage.o \
./GenericStage.o \
./InputStage.o \
./Locus.o \
./NetworkTransportStage.o \
./RoutingStage.o \
./Socket.o \
./StorageStage.o \
./TcpTransportStage.o \
./UdpTransportStage.o \
./Util.o \
./main.o 

CC_DEPS += \
./CommandBlock.d \
./DataBlock.d \
./FdSet.d \
./ForwardBlock.d \
./Socket.d 

CPP_DEPS += \
./AsyncStage.d \
./ChordDhtStage.d \
./EventInfo.d \
./EventState.d \
./ForwardingStage.d \
./GenericDhtStage.d \
./GenericStage.d \
./InputStage.d \
./Locus.d \
./NetworkTransportStage.d \
./RoutingStage.d \
./StorageStage.d \
./TcpTransportStage.d \
./UdpTransportStage.d \
./Util.d \
./main.d 


# Each subdirectory must supply rules for building sources it contributes
%.o: ../%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o"$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

%.o: ../%.cc
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o"$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


