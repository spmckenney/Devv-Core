################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/support/cleanse.cpp \
../src/support/lockedpool.cpp 

OBJS += \
./src/support/cleanse.o \
./src/support/lockedpool.o 

CPP_DEPS += \
./src/support/cleanse.d \
./src/support/lockedpool.d 


# Each subdirectory must supply rules for building sources it contributes
src/support/%.o: ../src/support/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: Cygwin C++ Compiler'
	g++ -I"C:\bc\local\usr\include" -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


