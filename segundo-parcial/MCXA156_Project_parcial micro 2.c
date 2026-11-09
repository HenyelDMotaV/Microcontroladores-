//***********************************************************//
/*           Microcontroladores                              */
/*  Nombre:    Henyel Mota                                   */
/*  Matricula:  2020-10735                                   */
/*  Seccion:   Miercoles                                     */
/*  Practica:  Parcial No 2                                  */
/*  Fecha:     8/11/2026                                     */
//***********************************************************//

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include "board.h"
#include "pin_mux.h"
#include "clock_config.h"
#include "fsl_port.h"
#include "fsl_gpio.h"
#include "fsl_clock.h"
#include "fsl_reset.h"
#include "fsl_ctimer.h"
#include "fsl_lpadc.h"
#include "fsl_lpuart.h"

#define ADC_MAX 4095U
#define TX_SIZE 2048U
#define LINE_MAX 32U
static volatile uint32_t milliseconds;
static volatile uint32_t telemetry_events;
static uint32_t telemetry_done;
static uint32_t timer_hz;
static uint16_t adc_value;
static uint8_t pct, threshold = 70U;
static bool running = true, alarm, sample_valid;
static char tx[TX_SIZE];
static uint32_t tx_read, tx_write;
static volatile uint32_t tx_overflows;
static char line[LINE_MAX + 1U];
static uint32_t line_length;
static bool line_bad, pending_cr;
static uint32_t button_since;
static bool button_candidate, button_stable, button_armed;

static void AlarmWrite(bool on)
{
    alarm = on;
    GPIO_PinWrite(GPIO3, 12U, on ? 0U : 1U);
}
static void AlarmResetDecision(void)
{
    AlarmWrite(running && sample_valid && pct >= threshold);
}
static void SetRunning(bool value)
{
    if (!value) { running = false; AlarmWrite(false); }
    else if (!running) { running = true; AlarmResetDecision(); }
}
static void AlarmSample(void)
{
    if (!running) AlarmWrite(false);
    else if (pct >= threshold) AlarmWrite(true);
    else if (pct < (uint8_t)(threshold - 5U)) AlarmWrite(false);
}


static void SendLine(const char *s)
{
    uint32_t count = (uint32_t)strlen(s);
    uint32_t used = (tx_write + TX_SIZE - tx_read) % TX_SIZE;
    if (count > TX_SIZE - 1U - used) { ++tx_overflows; return; }
    for (uint32_t i = 0; i < count; ++i) {
        tx[tx_write] = s[i];
        tx_write = (tx_write + 1U) % TX_SIZE;
    }
}
static void TxService(void)
{

    if (tx_read != tx_write &&
        (LPUART_GetStatusFlags(LPUART0) & kLPUART_TxDataRegEmptyFlag)) {
        LPUART_WriteByte(LPUART0, (uint8_t)tx[tx_read]);
        tx_read = (tx_read + 1U) % TX_SIZE;
    }
}
static void Status(void)
{
    char out[112];
    snprintf(out, sizeof(out),
        "t=%lu;ADC=%u;PCT=%u;THR=%u;MODE=%s;ALARM=%u\r\n",
        (unsigned long)milliseconds, (unsigned)adc_value, (unsigned)pct,
        (unsigned)threshold, running ? "RUN" : "PAUSE", (unsigned)alarm);
    SendLine(out);
}
static void Command(void)
{
    if (!strcmp(line, "STATUS")) { Status(); return; }
    if (!strcmp(line, "START")) { SetRunning(true); SendLine("OK RUN\r\n"); return; }
    if (!strcmp(line, "STOP")) { SetRunning(false); SendLine("OK PAUSE\r\n"); return; }
    if (!strncmp(line, "THR ", 4U)) {
        const char *p = line + 4U;
        uint32_t number = 0U;
        if (*p == '\0') { SendLine("ERR\r\n"); return; }
        while (*p) {
            if (*p < '0' || *p > '9') { SendLine("ERR\r\n"); return; }
            number = number * 10U + (uint32_t)(*p++ - '0');

            if (number > 95U) { SendLine("ERR\r\n"); return; }
        }
        if (number < 10U) { SendLine("ERR\r\n"); return; }
        threshold = (uint8_t)number;
        AlarmResetDecision();
        char out[24];
        snprintf(out, sizeof(out), "OK THR=%u\r\n", (unsigned)threshold);
        SendLine(out);
        return;
    }
    SendLine("ERR\r\n");
}
static void ReceiveByte(uint8_t c)
{
    if (c == '\n') {
        if (line_bad) SendLine("ERR\r\n");
        else if (line_length) { line[line_length] = '\0'; Command(); }
        line_length = 0U; line_bad = false; pending_cr = false;
        return;
    }
    if (line_bad) return;

    if (pending_cr) { line_bad = true; return; }
    if (c == '\r') { pending_cr = true; return; }
    if (c < 32U || c > 126U || line_length == LINE_MAX) {
        line_bad = true; return;
    }
    line[line_length++] = (char)c;
}
static void RxService(void)
{

    for (uint32_t i = 0U; i < 16U; ++i) {
        if (!(LPUART_GetStatusFlags(LPUART0) & kLPUART_RxDataRegFullFlag)) break;
        ReceiveByte(LPUART_ReadByte(LPUART0));
    }
}
static void ButtonService(void)
{
    uint32_t now = milliseconds;
    bool pressed = GPIO_PinRead(GPIO1, 7U) == 0U;
    if (pressed != button_candidate) {
        button_candidate = pressed; button_since = now;
    }
    if ((uint32_t)(now - button_since) < 30U) return;
    if (button_candidate != button_stable) {
        button_stable = button_candidate;
        if (button_stable) {
            if (button_armed) { button_armed = false; SetRunning(!running); }
        } else button_armed = true;
    }

    if (!button_candidate && !button_stable) button_armed = true;
}
static void AdcService(void)
{
    lpadc_conv_result_t result;
#if defined(FSL_FEATURE_LPADC_FIFO_COUNT) && FSL_FEATURE_LPADC_FIFO_COUNT == 2U
    bool ready = LPADC_GetConvResult(ADC1, &result, 0U);
#else
    bool ready = LPADC_GetConvResult(ADC1, &result);
#endif
    if (ready) {

        adc_value = (uint16_t)((result.convValue >> 3U) & ADC_MAX);
        pct = (uint8_t)(((uint32_t)adc_value * 100U + ADC_MAX / 2U) / ADC_MAX);
        sample_valid = true;
        AlarmSample();
    }
}
static void TimerTick(uint32_t flags)
{
    (void)flags;
    static uint16_t phase;
    ++milliseconds;
    ++phase;
    if (phase % 100U == 0U) {

        GPIO_PortToggle(GPIO3, 1UL << 1U);
        LPADC_DoSoftwareTrigger(ADC1, 1U);
    }
    if (phase % 500U == 0U) GPIO_PortToggle(GPIO3, 1UL << 13U);
    if (phase == 1000U) { phase = 0U; ++telemetry_events; }

}
static ctimer_callback_t timer_callback = TimerTick;

static void GpioInit(void)
{
    CLOCK_EnableClock(kCLOCK_GatePORT1);
    CLOCK_EnableClock(kCLOCK_GatePORT3);
    CLOCK_EnableClock(kCLOCK_GateGPIO1);
    CLOCK_EnableClock(kCLOCK_GateGPIO3);
    RESET_ReleasePeripheralReset(kPORT3_RST_SHIFT_RSTn);
    RESET_ReleasePeripheralReset(kGPIO1_RST_SHIFT_RSTn);
    RESET_ReleasePeripheralReset(kGPIO3_RST_SHIFT_RSTn);
    port_pin_config_t output = {
        kPORT_PullDisable, kPORT_LowPullResistor, kPORT_FastSlewRate,
        kPORT_PassiveFilterDisable, kPORT_OpenDrainDisable,
        kPORT_LowDriveStrength, kPORT_NormalDriveStrength, kPORT_MuxAlt0,
        kPORT_InputBufferEnable, kPORT_InputNormal, kPORT_UnlockRegister
    };
    port_pin_config_t input = output;
    input.pullSelect = kPORT_PullUp;
    PORT_SetPinConfig(PORT1, 7U, &input);
    PORT_SetPinConfig(PORT3, 12U, &output);
    PORT_SetPinConfig(PORT3, 13U, &output);
    PORT_SetPinConfig(PORT3, 0U, &output);
    PORT_SetPinConfig(PORT3, 1U, &output);
    gpio_pin_config_t off = {kGPIO_DigitalOutput, 1U};
    gpio_pin_config_t low = {kGPIO_DigitalOutput, 0U};
    gpio_pin_config_t in = {kGPIO_DigitalInput, 0U};
    GPIO_PinInit(GPIO3, 12U, &off);
    GPIO_PinInit(GPIO3, 13U, &off);
    GPIO_PinInit(GPIO3, 0U, &off);
    GPIO_PinInit(GPIO3, 1U, &low);
    GPIO_PinInit(GPIO1, 7U, &in);
    button_candidate = button_stable = GPIO_PinRead(GPIO1, 7U) == 0U;
}
static void UartInit(void)
{
    CLOCK_AttachClk(kFRO12M_to_LPUART0);
    CLOCK_SetClockDiv(kCLOCK_DivLPUART0, 1U);
    RESET_ReleasePeripheralReset(kLPUART0_RST_SHIFT_RSTn);

    lpuart_config_t config;
    LPUART_GetDefaultConfig(&config);

    config.baudRate_Bps = 115200U;
    config.parityMode = kLPUART_ParityDisabled;
    config.dataBitsCount = kLPUART_EightDataBits;
    config.stopBitCount = kLPUART_OneStopBit;
    config.enableTx = true;
    config.enableRx = true;

    LPUART_Init(LPUART0, &config, 12000000U);
}
static void AdcInit(void)
{
    CLOCK_AttachClk(kFRO12M_to_ADC1);
    CLOCK_SetClockDiv(kCLOCK_DivADC1, 1U);
    RESET_ReleasePeripheralReset(kADC1_RST_SHIFT_RSTn);
    lpadc_config_t config;
    LPADC_GetDefaultConfig(&config);
    config.enableAnalogPreliminary = true;
    config.referenceVoltageSource = kLPADC_ReferenceVoltageAlt3;
#if defined(FSL_FEATURE_LPADC_HAS_CFG_PWRSEL) && FSL_FEATURE_LPADC_HAS_CFG_PWRSEL
    config.powerLevelMode = kLPADC_PowerLevelAlt4;
#endif
    LPADC_Init(ADC1, &config);
#if defined(FSL_FEATURE_LPADC_HAS_CTRL_CALOFS) && FSL_FEATURE_LPADC_HAS_CTRL_CALOFS
    LPADC_DoOffsetCalibration(ADC1);
#endif
#if (defined(FSL_FEATURE_LPADC_HAS_CTRL_CAL_REQ) && FSL_FEATURE_LPADC_HAS_CTRL_CAL_REQ) || \
    (defined(FSL_FEATURE_LPADC_HAS_CFG_CALOFS) && FSL_FEATURE_LPADC_HAS_CFG_CALOFS)

    SDK_DelayAtLeastUs(1U, SystemCoreClock);
    LPADC_DoAutoCalibration(ADC1);
#endif
    lpadc_conv_command_config_t command;
    LPADC_GetDefaultConvCommandConfig(&command);
    command.channelNumber = 8U;
    command.conversionResolutionMode = kLPADC_ConversionResolutionStandard;
    LPADC_SetConvCommandConfig(ADC1, 1U, &command);
    lpadc_conv_trigger_config_t trigger;
    LPADC_GetDefaultConvTriggerConfig(&trigger);
    trigger.targetCommandId = 1U;
    trigger.enableHardwareTrigger = false;
    LPADC_SetConvTriggerConfig(ADC1, 0U, &trigger);
}
static void TimerInit(void)
{

    CLOCK_AttachClk(kFRO_HF_to_CTIMER1);
    CLOCK_SetClockDiv(kCLOCK_DivCTIMER1, 1U);
    ctimer_config_t config;
    CTIMER_GetDefaultConfig(&config);
    config.prescale = 0U;
    CTIMER_Init(CTIMER1, &config);
    timer_hz = CLOCK_GetCTimerClkFreq(1U);
    ctimer_match_config_t match = {0};
    match.enableCounterReset = true;
    match.enableCounterStop = false;
    match.matchValue = timer_hz / 1000U;
    match.outControl = kCTIMER_Output_NoAction;
    match.outPinInitState = false;
    match.enableInterrupt = true;
    CTIMER_RegisterCallBack(CTIMER1, &timer_callback, kCTIMER_SingleCallback);
    CTIMER_SetupMatch(CTIMER1, kCTIMER_Match_0, &match);
    CTIMER_StartTimer(CTIMER1);
}
int main(void)
{

    BOARD_InitBootPins();
    BOARD_InitBootClocks();
    GpioInit();
    UartInit();
    AdcInit();
    TimerInit();
    for (;;) {
        AdcService();
        ButtonService();
        RxService();
        if (telemetry_done != telemetry_events) {
            ++telemetry_done;
            Status();
        }
        TxService();
    }
}
