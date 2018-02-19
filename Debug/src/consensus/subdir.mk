################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/consensus/blockmanager.cpp \
../src/consensus/merkle.cpp \
../src/consensus/statestub.cpp 

OBJS += \
./src/consensus/blockmanager.o \
./src/consensus/merkle.o \
./src/consensus/statestub.o 

CPP_DEPS += \
./src/consensus/blockmanager.d \
./src/consensus/merkle.d \
./src/consensus/statestub.d 


# Each subdirectory must supply rules for building sources it contributes
src/consensus/%.o: ../src/consensus/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: Cygwin C++ Compiler'
	g++ -I"C:\bc\local\usr\include" -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


