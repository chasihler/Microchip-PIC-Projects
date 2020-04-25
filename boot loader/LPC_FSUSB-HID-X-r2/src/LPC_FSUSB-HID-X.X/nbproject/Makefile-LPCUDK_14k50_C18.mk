#
# Generated Makefile - do not edit!
#
# Edit the Makefile in the project folder instead (../Makefile). Each target
# has a -pre and a -post target defined where you can add customized code.
#
# This makefile implements configuration specific macros and targets.


# Include project Makefile
ifeq "${IGNORE_LOCAL}" "TRUE"
# do not include local makefile. User is passing all local related variables already
else
include Makefile
# Include makefile containing local settings
ifeq "$(wildcard nbproject/Makefile-local-LPCUDK_14k50_C18.mk)" "nbproject/Makefile-local-LPCUDK_14k50_C18.mk"
include nbproject/Makefile-local-LPCUDK_14k50_C18.mk
endif
endif

# Environment
MKDIR=gnumkdir -p
RM=rm -f 
MV=mv 
CP=cp 

# Macros
CND_CONF=LPCUDK_14k50_C18
ifeq ($(TYPE_IMAGE), DEBUG_RUN)
IMAGE_TYPE=debug
OUTPUT_SUFFIX=cof
DEBUGGABLE_SUFFIX=cof
FINAL_IMAGE=dist/${CND_CONF}/${IMAGE_TYPE}/LPC_FSUSB-HID-X.X.${IMAGE_TYPE}.${OUTPUT_SUFFIX}
else
IMAGE_TYPE=production
OUTPUT_SUFFIX=hex
DEBUGGABLE_SUFFIX=cof
FINAL_IMAGE=dist/${CND_CONF}/${IMAGE_TYPE}/LPC_FSUSB-HID-X.X.${IMAGE_TYPE}.${OUTPUT_SUFFIX}
endif

# Object Directory
OBJECTDIR=build/${CND_CONF}/${IMAGE_TYPE}

# Distribution Directory
DISTDIR=dist/${CND_CONF}/${IMAGE_TYPE}

# Source Files Quoted if spaced
SOURCEFILES_QUOTED_IF_SPACED=usb_descriptors.c main_hid.c system.c app_custom_hid.c ../BSP-FILES/lpcudk_14K50.c ../mla/v2014_07_22/framework/usb/src/usb_device.c ../mla/v2014_07_22/framework/usb/src/usb_device_hid.c

# Object Files Quoted if spaced
OBJECTFILES_QUOTED_IF_SPACED=${OBJECTDIR}/usb_descriptors.o ${OBJECTDIR}/main_hid.o ${OBJECTDIR}/system.o ${OBJECTDIR}/app_custom_hid.o ${OBJECTDIR}/_ext/1126228070/lpcudk_14K50.o ${OBJECTDIR}/_ext/164277197/usb_device.o ${OBJECTDIR}/_ext/164277197/usb_device_hid.o
POSSIBLE_DEPFILES=${OBJECTDIR}/usb_descriptors.o.d ${OBJECTDIR}/main_hid.o.d ${OBJECTDIR}/system.o.d ${OBJECTDIR}/app_custom_hid.o.d ${OBJECTDIR}/_ext/1126228070/lpcudk_14K50.o.d ${OBJECTDIR}/_ext/164277197/usb_device.o.d ${OBJECTDIR}/_ext/164277197/usb_device_hid.o.d

# Object Files
OBJECTFILES=${OBJECTDIR}/usb_descriptors.o ${OBJECTDIR}/main_hid.o ${OBJECTDIR}/system.o ${OBJECTDIR}/app_custom_hid.o ${OBJECTDIR}/_ext/1126228070/lpcudk_14K50.o ${OBJECTDIR}/_ext/164277197/usb_device.o ${OBJECTDIR}/_ext/164277197/usb_device_hid.o

# Source Files
SOURCEFILES=usb_descriptors.c main_hid.c system.c app_custom_hid.c ../BSP-FILES/lpcudk_14K50.c ../mla/v2014_07_22/framework/usb/src/usb_device.c ../mla/v2014_07_22/framework/usb/src/usb_device_hid.c


CFLAGS=
ASFLAGS=
LDLIBSOPTIONS=

############# Tool locations ##########################################
# If you copy a project from one host to another, the path where the  #
# compiler is installed may be different.                             #
# If you open this project with MPLAB X in the new host, this         #
# makefile will be regenerated and the paths will be corrected.       #
#######################################################################
# fixDeps replaces a bunch of sed/cat/printf statements that slow down the build
FIXDEPS=fixDeps

.build-conf:  ${BUILD_SUBPROJECTS}
ifneq ($(INFORMATION_MESSAGE), )
	@echo $(INFORMATION_MESSAGE)
endif
	${MAKE}  -f nbproject/Makefile-LPCUDK_14k50_C18.mk dist/${CND_CONF}/${IMAGE_TYPE}/LPC_FSUSB-HID-X.X.${IMAGE_TYPE}.${OUTPUT_SUFFIX}

MP_PROCESSOR_OPTION=18F25K50
MP_PROCESSOR_OPTION_LD=18f25k50
MP_LINKER_DEBUG_OPTION=  -u_DEBUGSTACK
# ------------------------------------------------------------------------------------
# Rules for buildStep: assemble
ifeq ($(TYPE_IMAGE), DEBUG_RUN)
else
endif

# ------------------------------------------------------------------------------------
# Rules for buildStep: compile
ifeq ($(TYPE_IMAGE), DEBUG_RUN)
${OBJECTDIR}/usb_descriptors.o: usb_descriptors.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} ${OBJECTDIR} 
	@${RM} ${OBJECTDIR}/usb_descriptors.o.d 
	@${RM} ${OBJECTDIR}/usb_descriptors.o 
	${MP_CC} $(MP_EXTRA_CC_PRE) -D__DEBUG -D__MPLAB_DEBUGGER_PK3=1 -p$(MP_PROCESSOR_OPTION) -DCOMPILER_MPLAB_C18 -I"../mla/v2014_07_22/framework" -I"./" -ms -oa-  -I ${MP_CC_DIR}\\..\\h  -fo ${OBJECTDIR}/usb_descriptors.o   usb_descriptors.c 
	@${DEP_GEN} -d ${OBJECTDIR}/usb_descriptors.o 
	@${FIXDEPS} "${OBJECTDIR}/usb_descriptors.o.d" $(SILENT) -rsi ${MP_CC_DIR}../ -c18 
	
${OBJECTDIR}/main_hid.o: main_hid.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} ${OBJECTDIR} 
	@${RM} ${OBJECTDIR}/main_hid.o.d 
	@${RM} ${OBJECTDIR}/main_hid.o 
	${MP_CC} $(MP_EXTRA_CC_PRE) -D__DEBUG -D__MPLAB_DEBUGGER_PK3=1 -p$(MP_PROCESSOR_OPTION) -DCOMPILER_MPLAB_C18 -I"../mla/v2014_07_22/framework" -I"./" -ms -oa-  -I ${MP_CC_DIR}\\..\\h  -fo ${OBJECTDIR}/main_hid.o   main_hid.c 
	@${DEP_GEN} -d ${OBJECTDIR}/main_hid.o 
	@${FIXDEPS} "${OBJECTDIR}/main_hid.o.d" $(SILENT) -rsi ${MP_CC_DIR}../ -c18 
	
${OBJECTDIR}/system.o: system.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} ${OBJECTDIR} 
	@${RM} ${OBJECTDIR}/system.o.d 
	@${RM} ${OBJECTDIR}/system.o 
	${MP_CC} $(MP_EXTRA_CC_PRE) -D__DEBUG -D__MPLAB_DEBUGGER_PK3=1 -p$(MP_PROCESSOR_OPTION) -DCOMPILER_MPLAB_C18 -I"../mla/v2014_07_22/framework" -I"./" -ms -oa-  -I ${MP_CC_DIR}\\..\\h  -fo ${OBJECTDIR}/system.o   system.c 
	@${DEP_GEN} -d ${OBJECTDIR}/system.o 
	@${FIXDEPS} "${OBJECTDIR}/system.o.d" $(SILENT) -rsi ${MP_CC_DIR}../ -c18 
	
${OBJECTDIR}/app_custom_hid.o: app_custom_hid.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} ${OBJECTDIR} 
	@${RM} ${OBJECTDIR}/app_custom_hid.o.d 
	@${RM} ${OBJECTDIR}/app_custom_hid.o 
	${MP_CC} $(MP_EXTRA_CC_PRE) -D__DEBUG -D__MPLAB_DEBUGGER_PK3=1 -p$(MP_PROCESSOR_OPTION) -DCOMPILER_MPLAB_C18 -I"../mla/v2014_07_22/framework" -I"./" -ms -oa-  -I ${MP_CC_DIR}\\..\\h  -fo ${OBJECTDIR}/app_custom_hid.o   app_custom_hid.c 
	@${DEP_GEN} -d ${OBJECTDIR}/app_custom_hid.o 
	@${FIXDEPS} "${OBJECTDIR}/app_custom_hid.o.d" $(SILENT) -rsi ${MP_CC_DIR}../ -c18 
	
${OBJECTDIR}/_ext/1126228070/lpcudk_14K50.o: ../BSP-FILES/lpcudk_14K50.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} ${OBJECTDIR}/_ext/1126228070 
	@${RM} ${OBJECTDIR}/_ext/1126228070/lpcudk_14K50.o.d 
	@${RM} ${OBJECTDIR}/_ext/1126228070/lpcudk_14K50.o 
	${MP_CC} $(MP_EXTRA_CC_PRE) -D__DEBUG -D__MPLAB_DEBUGGER_PK3=1 -p$(MP_PROCESSOR_OPTION) -DCOMPILER_MPLAB_C18 -I"../mla/v2014_07_22/framework" -I"./" -ms -oa-  -I ${MP_CC_DIR}\\..\\h  -fo ${OBJECTDIR}/_ext/1126228070/lpcudk_14K50.o   ../BSP-FILES/lpcudk_14K50.c 
	@${DEP_GEN} -d ${OBJECTDIR}/_ext/1126228070/lpcudk_14K50.o 
	@${FIXDEPS} "${OBJECTDIR}/_ext/1126228070/lpcudk_14K50.o.d" $(SILENT) -rsi ${MP_CC_DIR}../ -c18 
	
${OBJECTDIR}/_ext/164277197/usb_device.o: ../mla/v2014_07_22/framework/usb/src/usb_device.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} ${OBJECTDIR}/_ext/164277197 
	@${RM} ${OBJECTDIR}/_ext/164277197/usb_device.o.d 
	@${RM} ${OBJECTDIR}/_ext/164277197/usb_device.o 
	${MP_CC} $(MP_EXTRA_CC_PRE) -D__DEBUG -D__MPLAB_DEBUGGER_PK3=1 -p$(MP_PROCESSOR_OPTION) -DCOMPILER_MPLAB_C18 -I"../mla/v2014_07_22/framework" -I"./" -ms -oa-  -I ${MP_CC_DIR}\\..\\h  -fo ${OBJECTDIR}/_ext/164277197/usb_device.o   ../mla/v2014_07_22/framework/usb/src/usb_device.c 
	@${DEP_GEN} -d ${OBJECTDIR}/_ext/164277197/usb_device.o 
	@${FIXDEPS} "${OBJECTDIR}/_ext/164277197/usb_device.o.d" $(SILENT) -rsi ${MP_CC_DIR}../ -c18 
	
${OBJECTDIR}/_ext/164277197/usb_device_hid.o: ../mla/v2014_07_22/framework/usb/src/usb_device_hid.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} ${OBJECTDIR}/_ext/164277197 
	@${RM} ${OBJECTDIR}/_ext/164277197/usb_device_hid.o.d 
	@${RM} ${OBJECTDIR}/_ext/164277197/usb_device_hid.o 
	${MP_CC} $(MP_EXTRA_CC_PRE) -D__DEBUG -D__MPLAB_DEBUGGER_PK3=1 -p$(MP_PROCESSOR_OPTION) -DCOMPILER_MPLAB_C18 -I"../mla/v2014_07_22/framework" -I"./" -ms -oa-  -I ${MP_CC_DIR}\\..\\h  -fo ${OBJECTDIR}/_ext/164277197/usb_device_hid.o   ../mla/v2014_07_22/framework/usb/src/usb_device_hid.c 
	@${DEP_GEN} -d ${OBJECTDIR}/_ext/164277197/usb_device_hid.o 
	@${FIXDEPS} "${OBJECTDIR}/_ext/164277197/usb_device_hid.o.d" $(SILENT) -rsi ${MP_CC_DIR}../ -c18 
	
else
${OBJECTDIR}/usb_descriptors.o: usb_descriptors.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} ${OBJECTDIR} 
	@${RM} ${OBJECTDIR}/usb_descriptors.o.d 
	@${RM} ${OBJECTDIR}/usb_descriptors.o 
	${MP_CC} $(MP_EXTRA_CC_PRE) -p$(MP_PROCESSOR_OPTION) -DCOMPILER_MPLAB_C18 -I"../mla/v2014_07_22/framework" -I"./" -ms -oa-  -I ${MP_CC_DIR}\\..\\h  -fo ${OBJECTDIR}/usb_descriptors.o   usb_descriptors.c 
	@${DEP_GEN} -d ${OBJECTDIR}/usb_descriptors.o 
	@${FIXDEPS} "${OBJECTDIR}/usb_descriptors.o.d" $(SILENT) -rsi ${MP_CC_DIR}../ -c18 
	
${OBJECTDIR}/main_hid.o: main_hid.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} ${OBJECTDIR} 
	@${RM} ${OBJECTDIR}/main_hid.o.d 
	@${RM} ${OBJECTDIR}/main_hid.o 
	${MP_CC} $(MP_EXTRA_CC_PRE) -p$(MP_PROCESSOR_OPTION) -DCOMPILER_MPLAB_C18 -I"../mla/v2014_07_22/framework" -I"./" -ms -oa-  -I ${MP_CC_DIR}\\..\\h  -fo ${OBJECTDIR}/main_hid.o   main_hid.c 
	@${DEP_GEN} -d ${OBJECTDIR}/main_hid.o 
	@${FIXDEPS} "${OBJECTDIR}/main_hid.o.d" $(SILENT) -rsi ${MP_CC_DIR}../ -c18 
	
${OBJECTDIR}/system.o: system.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} ${OBJECTDIR} 
	@${RM} ${OBJECTDIR}/system.o.d 
	@${RM} ${OBJECTDIR}/system.o 
	${MP_CC} $(MP_EXTRA_CC_PRE) -p$(MP_PROCESSOR_OPTION) -DCOMPILER_MPLAB_C18 -I"../mla/v2014_07_22/framework" -I"./" -ms -oa-  -I ${MP_CC_DIR}\\..\\h  -fo ${OBJECTDIR}/system.o   system.c 
	@${DEP_GEN} -d ${OBJECTDIR}/system.o 
	@${FIXDEPS} "${OBJECTDIR}/system.o.d" $(SILENT) -rsi ${MP_CC_DIR}../ -c18 
	
${OBJECTDIR}/app_custom_hid.o: app_custom_hid.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} ${OBJECTDIR} 
	@${RM} ${OBJECTDIR}/app_custom_hid.o.d 
	@${RM} ${OBJECTDIR}/app_custom_hid.o 
	${MP_CC} $(MP_EXTRA_CC_PRE) -p$(MP_PROCESSOR_OPTION) -DCOMPILER_MPLAB_C18 -I"../mla/v2014_07_22/framework" -I"./" -ms -oa-  -I ${MP_CC_DIR}\\..\\h  -fo ${OBJECTDIR}/app_custom_hid.o   app_custom_hid.c 
	@${DEP_GEN} -d ${OBJECTDIR}/app_custom_hid.o 
	@${FIXDEPS} "${OBJECTDIR}/app_custom_hid.o.d" $(SILENT) -rsi ${MP_CC_DIR}../ -c18 
	
${OBJECTDIR}/_ext/1126228070/lpcudk_14K50.o: ../BSP-FILES/lpcudk_14K50.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} ${OBJECTDIR}/_ext/1126228070 
	@${RM} ${OBJECTDIR}/_ext/1126228070/lpcudk_14K50.o.d 
	@${RM} ${OBJECTDIR}/_ext/1126228070/lpcudk_14K50.o 
	${MP_CC} $(MP_EXTRA_CC_PRE) -p$(MP_PROCESSOR_OPTION) -DCOMPILER_MPLAB_C18 -I"../mla/v2014_07_22/framework" -I"./" -ms -oa-  -I ${MP_CC_DIR}\\..\\h  -fo ${OBJECTDIR}/_ext/1126228070/lpcudk_14K50.o   ../BSP-FILES/lpcudk_14K50.c 
	@${DEP_GEN} -d ${OBJECTDIR}/_ext/1126228070/lpcudk_14K50.o 
	@${FIXDEPS} "${OBJECTDIR}/_ext/1126228070/lpcudk_14K50.o.d" $(SILENT) -rsi ${MP_CC_DIR}../ -c18 
	
${OBJECTDIR}/_ext/164277197/usb_device.o: ../mla/v2014_07_22/framework/usb/src/usb_device.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} ${OBJECTDIR}/_ext/164277197 
	@${RM} ${OBJECTDIR}/_ext/164277197/usb_device.o.d 
	@${RM} ${OBJECTDIR}/_ext/164277197/usb_device.o 
	${MP_CC} $(MP_EXTRA_CC_PRE) -p$(MP_PROCESSOR_OPTION) -DCOMPILER_MPLAB_C18 -I"../mla/v2014_07_22/framework" -I"./" -ms -oa-  -I ${MP_CC_DIR}\\..\\h  -fo ${OBJECTDIR}/_ext/164277197/usb_device.o   ../mla/v2014_07_22/framework/usb/src/usb_device.c 
	@${DEP_GEN} -d ${OBJECTDIR}/_ext/164277197/usb_device.o 
	@${FIXDEPS} "${OBJECTDIR}/_ext/164277197/usb_device.o.d" $(SILENT) -rsi ${MP_CC_DIR}../ -c18 
	
${OBJECTDIR}/_ext/164277197/usb_device_hid.o: ../mla/v2014_07_22/framework/usb/src/usb_device_hid.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} ${OBJECTDIR}/_ext/164277197 
	@${RM} ${OBJECTDIR}/_ext/164277197/usb_device_hid.o.d 
	@${RM} ${OBJECTDIR}/_ext/164277197/usb_device_hid.o 
	${MP_CC} $(MP_EXTRA_CC_PRE) -p$(MP_PROCESSOR_OPTION) -DCOMPILER_MPLAB_C18 -I"../mla/v2014_07_22/framework" -I"./" -ms -oa-  -I ${MP_CC_DIR}\\..\\h  -fo ${OBJECTDIR}/_ext/164277197/usb_device_hid.o   ../mla/v2014_07_22/framework/usb/src/usb_device_hid.c 
	@${DEP_GEN} -d ${OBJECTDIR}/_ext/164277197/usb_device_hid.o 
	@${FIXDEPS} "${OBJECTDIR}/_ext/164277197/usb_device_hid.o.d" $(SILENT) -rsi ${MP_CC_DIR}../ -c18 
	
endif

# ------------------------------------------------------------------------------------
# Rules for buildStep: link
ifeq ($(TYPE_IMAGE), DEBUG_RUN)
dist/${CND_CONF}/${IMAGE_TYPE}/LPC_FSUSB-HID-X.X.${IMAGE_TYPE}.${OUTPUT_SUFFIX}: ${OBJECTFILES}  nbproject/Makefile-${CND_CONF}.mk    
	@${MKDIR} dist/${CND_CONF}/${IMAGE_TYPE} 
	${MP_LD} $(MP_EXTRA_LD_PRE)   -p$(MP_PROCESSOR_OPTION_LD)  -w -x -u_DEBUG -m"${DISTDIR}/${PROJECTNAME}.${IMAGE_TYPE}.map"  -z__MPLAB_BUILD=1  -u_CRUNTIME -z__MPLAB_DEBUG=1 -z__MPLAB_DEBUGGER_PK3=1 $(MP_LINKER_DEBUG_OPTION) -l ${MP_CC_DIR}\\..\\lib  -o dist/${CND_CONF}/${IMAGE_TYPE}/LPC_FSUSB-HID-X.X.${IMAGE_TYPE}.${OUTPUT_SUFFIX}  ${OBJECTFILES_QUOTED_IF_SPACED}   
else
dist/${CND_CONF}/${IMAGE_TYPE}/LPC_FSUSB-HID-X.X.${IMAGE_TYPE}.${OUTPUT_SUFFIX}: ${OBJECTFILES}  nbproject/Makefile-${CND_CONF}.mk   
	@${MKDIR} dist/${CND_CONF}/${IMAGE_TYPE} 
	${MP_LD} $(MP_EXTRA_LD_PRE)   -p$(MP_PROCESSOR_OPTION_LD)  -w  -m"${DISTDIR}/${PROJECTNAME}.${IMAGE_TYPE}.map"  -z__MPLAB_BUILD=1  -u_CRUNTIME -l ${MP_CC_DIR}\\..\\lib  -o dist/${CND_CONF}/${IMAGE_TYPE}/LPC_FSUSB-HID-X.X.${IMAGE_TYPE}.${DEBUGGABLE_SUFFIX}  ${OBJECTFILES_QUOTED_IF_SPACED}   
endif


# Subprojects
.build-subprojects:


# Subprojects
.clean-subprojects:

# Clean Targets
.clean-conf: ${CLEAN_SUBPROJECTS}
	${RM} -r build/LPCUDK_14k50_C18
	${RM} -r dist/LPCUDK_14k50_C18

# Enable dependency checking
.dep.inc: .depcheck-impl

DEPFILES=$(shell mplabwildcard ${POSSIBLE_DEPFILES})
ifneq (${DEPFILES},)
include ${DEPFILES}
endif
