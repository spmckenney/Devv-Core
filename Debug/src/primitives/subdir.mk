################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/primitives/block.cpp \
../src/primitives/transaction.cpp \
../src/primitives/validation.cpp 

OBJS += \
./src/primitives/block.o \
./src/primitives/transaction.o \
./src/primitives/validation.o 

CPP_DEPS += \
./src/primitives/block.d \
./src/primitives/transaction.d \
./src/primitives/validation.d 


# Each subdirectory must supply rules for building sources it contributes
src/primitives/%.o: ../src/primitives/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: Cygwin C++ Compiler'
	g++ -I"C:\bc\local\usr\include" -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


