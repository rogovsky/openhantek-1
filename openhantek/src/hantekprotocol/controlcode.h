#pragma once


//////////////////////////////////////////////////////////////////////////////
/// \enum ControlCode                                           hantek/types.h
/// \brief All supported control commands.
enum ControlCode {
    /// <em>[::MODEL_DSO2090, ::MODEL_DSO2150, ::MODEL_DSO2250, ::MODEL_DSO5200,
    /// ::MODEL_DSO5200A]</em>
    /// <p>
    ///   The 0xa2 control read/write command gives access to a ::ControlValue.
    /// </p>
    /// <p><br /></p>
    CONTROL_VALUE = 0xa2,

    /// <em>[::MODEL_DSO2090, ::MODEL_DSO2150, ::MODEL_DSO2250, ::MODEL_DSO5200,
    /// ::MODEL_DSO5200A]</em>
    /// <p>
    ///   The 0xb2 control read command gets the speed level of the USB
    ///   connection:
    ///   <table>
    ///     <tr>
    ///       <td>::ConnectionSpeed</td>
    ///       <td>0x00</td>
    ///       <td>0x00</td>
    ///       <td>0x00</td>
    ///       <td>0x00</td>
    ///       <td>0x00</td>
    ///       <td>0x00</td>
    ///       <td>0x00</td>
    ///       <td>0x00</td>
    ///       <td>0x00</td>
    ///     </tr>
    ///   </table>
    /// </p>
    /// <p><br /></p>
    CONTROL_GETSPEED = 0xb2,

    /// <em>[::MODEL_DSO2090, ::MODEL_DSO2150, ::MODEL_DSO2250, ::MODEL_DSO5200,
    /// ::MODEL_DSO5200A]</em>
    /// <p>
    ///   The 0xb3 control write command is sent before any bulk command:
    ///   <table>
    ///     <tr>
    ///       <td>0x0f</td>
    ///       <td>::BulkIndex</td>
    ///       <td>::BulkIndex</td>
    ///       <td>::BulkIndex</td>
    ///       <td>0x00</td>
    ///       <td>0x00</td>
    ///       <td>0x00</td>
    ///       <td>0x00</td>
    ///       <td>0x00</td>
    ///       <td>0x00</td>
    ///     </tr>
    ///   </table>
    /// </p>
    /// <p><br /></p>
    CONTROL_BEGINCOMMAND = 0xb3,

    /// <em>[::MODEL_DSO2090, ::MODEL_DSO2150, ::MODEL_DSO2250, ::MODEL_DSO5200,
    /// ::MODEL_DSO5200A]</em>
    /// <p>
    ///   The 0xb4 control write command sets the channel offsets:
    ///   <table>
    ///     <tr>
    ///       <td>Ch1Offset[1]</td>
    ///       <td>Ch1Offset[0]</td>
    ///       <td>Ch2Offset[1]</td>
    ///       <td>Ch2Offset[0]</td>
    ///       <td>TriggerOffset[1]</td>
    ///       <td>TriggerOffset[0]</td>
    ///     </tr>
    ///   </table>
    ///   <table>
    ///     <tr>
    ///       <td>0x00</td>
    ///       <td>0x00</td>
    ///       <td>0x00</td>
    ///       <td>0x00</td>
    ///       <td>0x00</td>
    ///       <td>0x00</td>
    ///       <td>0x00</td>
    ///       <td>0x00</td>
    ///       <td>0x00</td>
    ///       <td>0x00</td>
    ///       <td>0x00</td>
    ///     </tr>
    ///   </table>
    /// </p>
    /// <p><br /></p>
    CONTROL_SETOFFSET = 0xb4,

    /// <em>[::MODEL_DSO2090, ::MODEL_DSO2150, ::MODEL_DSO2250, ::MODEL_DSO5200,
    /// ::MODEL_DSO5200A]</em>
    /// <p>
    ///   The 0xb5 control write command sets the internal relays:
    ///   <table>
    ///     <tr>
    ///       <td>0x00</td>
    ///       <td>0x04 ^ (Ch1Gain < 1 V)</td>
    ///       <td>0x08 ^ (Ch1Gain < 100 mV)</td>
    ///       <td>0x02 ^ (Ch1Coupling == DC)</td>
    ///     </tr>
    ///   </table>
    ///   <table>
    ///     <tr>
    ///       <td>0x20 ^ (Ch2Gain < 1 V)</td>
    ///       <td>0x40 ^ (Ch2Gain < 100 mV)</td>
    ///       <td>0x10 ^ (Ch2Coupling == DC)</td>
    ///       <td>0x01 ^ (Trigger == EXT)</td>
    ///     </tr>
    ///   </table>
    ///   <table>
    ///     <tr>
    ///       <td>0x00</td>
    ///       <td>0x00</td>
    ///       <td>0x00</td>
    ///       <td>0x00</td>
    ///       <td>0x00</td>
    ///       <td>0x00</td>
    ///       <td>0x00</td>
    ///       <td>0x00</td>
    ///       <td>0x00</td>
    ///     </tr>
    ///   </table>
    /// </p>
    /// <p>
    ///   The limits are <= instead of < for the 10 bit models, since those
    ///   support voltages up to 10 V.
    /// </p>
    /// <p><br /></p>
    CONTROL_SETRELAYS = 0xb5,

    CONTROL_SETVOLTDIV_CH1 = 0xe0,
    CONTROL_SETVOLTDIV_CH2 = 0xe1,
    CONTROL_SETTIMEDIV = 0xe2,
    CONTROL_ACQUIIRE_HARD_DATA = 0xe3
};

