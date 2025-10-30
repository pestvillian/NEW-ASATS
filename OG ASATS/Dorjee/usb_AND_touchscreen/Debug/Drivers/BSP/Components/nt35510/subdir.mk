################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (12.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Drivers/BSP/Components/nt35510/nt35510.c 

OBJS += \
./Drivers/BSP/Components/nt35510/nt35510.o 

C_DEPS += \
./Drivers/BSP/Components/nt35510/nt35510.d 


# Each subdirectory must supply rules for building sources it contributes
Drivers/BSP/Components/nt35510/%.o Drivers/BSP/Components/nt35510/%.su Drivers/BSP/Components/nt35510/%.cyclo: ../Drivers/BSP/Components/nt35510/%.c Drivers/BSP/Components/nt35510/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32F429xx -c -I"C:/Users/Dtenz/STM32CubeIDE/workspace_1.17.0/usb_AND_touchscreen/Drivers/BSP/STM32F429I-Discovery" -I"C:/Users/Dtenz/STM32CubeIDE/workspace_1.17.0/usb_AND_touchscreen/Utilities" -I../Core/Inc -I../USB_HOST/App -I../USB_HOST/Target -I../Drivers/STM32F4xx_HAL_Driver/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc/Legacy -I../Middlewares/ST/STM32_USB_Host_Library/Core/Inc -I../Drivers/CMSIS/Device/ST/STM32F4xx/Include -I../Drivers/CMSIS/Include -I../Middlewares/ST/STM32_USB_Host_Library/Class/HID/Inc -I"C:/Users/Dtenz/STM32CubeIDE/workspace_1.17.0/usb_AND_touchscreen/Drivers/BSP" -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-Drivers-2f-BSP-2f-Components-2f-nt35510

clean-Drivers-2f-BSP-2f-Components-2f-nt35510:
	-$(RM) ./Drivers/BSP/Components/nt35510/nt35510.cyclo ./Drivers/BSP/Components/nt35510/nt35510.d ./Drivers/BSP/Components/nt35510/nt35510.o ./Drivers/BSP/Components/nt35510/nt35510.su

.PHONY: clean-Drivers-2f-BSP-2f-Components-2f-nt35510

