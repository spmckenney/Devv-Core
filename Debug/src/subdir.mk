################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/devcash.cpp \
../src/init.cpp \
../src/sync.cpp \
../src/uint256.cpp \
../src/util.cpp \
../src/utilstrencodings.cpp \
../src/utiltime.cpp 

OBJS += \
./src/devcash.o \
./src/init.o \
./src/sync.o \
./src/uint256.o \
./src/util.o \
./src/utilstrencodings.o \
./src/utiltime.o 

CPP_DEPS += \
./src/devcash.d \
./src/init.d \
./src/sync.d \
./src/uint256.d \
./src/util.d \
./src/utilstrencodings.d \
./src/utiltime.d 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: Cygwin C++ Compiler'
	g++ -I"C:\bc\local\usr\include" -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


