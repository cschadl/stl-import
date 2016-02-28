################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../stl_import.cpp \
../stl_importer.cpp \
../stl_importer_tests.cpp \
../test.cpp \
../triangle_mesh.cpp \
../tut_test.cpp 

OBJS += \
./stl_import.o \
./stl_importer.o \
./stl_importer_tests.o \
./test.o \
./triangle_mesh.o \
./tut_test.o 

CPP_DEPS += \
./stl_import.d \
./stl_importer.d \
./stl_importer_tests.d \
./test.d \
./triangle_mesh.d \
./tut_test.d 


# Each subdirectory must supply rules for building sources it contributes
%.o: ../%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -std=c++11 -I"/home/chris/workspace/mathstuff" -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


