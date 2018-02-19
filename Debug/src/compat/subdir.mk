################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/compat/strnlen.cpp 

OBJS += \
./src/compat/strnlen.o 

CPP_DEPS += \
./src/compat/strnlen.d 


# Each subdirectory must supply rules for building sources it contributes
src/compat/%.o: ../src/compat/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: Cygwin C++ Compiler'
	g++ -I"C:\bc\local\usr\include" -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


