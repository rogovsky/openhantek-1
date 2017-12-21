#pragma once

namespace Hantek {

//////////////////////////////////////////////////////////////////////////////
/// \enum BulkCode                                              hantek/types.h
/// \brief All supported bulk commands.
/// Indicies given in square brackets specify byte numbers in little endian
/// format.
enum BulkCode {
    /// BulkSetFilter [<em>::MODEL_DSO2090, ::MODEL_DSO2150, ::MODEL_DSO5200,
    /// ::MODEL_DSO5200A</em>]
    /// <p>
    ///   This command sets channel and trigger filter:
    ///   <table>
    ///     <tr>
    ///       <td>0x00</td>
    ///       <td>0x00</td>
    ///       <td>FilterBits</td>
    ///       <td>0x00</td>
    ///       <td>0x00</td>
    ///       <td>0x00</td>
    ///       <td>0x00</td>
    ///       <td>0x00</td>
    ///     </tr>
    ///   </table>
    /// </p>
    /// <p>
    ///   This command is used by the official %Hantek software, but doesn't seem
    ///   to be used by the device.
    /// <p><br /></p>
    BULK_SETFILTER,

    /// BulkSetTriggerAndSamplerate [<em>::MODEL_DSO2090, ::MODEL_DSO2150</em>]
    /// <p>
    ///   This command sets trigger and timebase:
    ///   <table>
    ///     <tr>
    ///       <td>0x01</td>
    ///       <td>0x00</td>
    ///       <td>Tsr1Bits</td>
    ///       <td>Tsr2Bits</td>
    ///       <td>Downsampler[0]</td>
    ///       <td>Downsampler[1]</td>
    ///     </tr>
    ///   </table>
    ///   <table>
    ///     <tr>
    ///       <td>TriggerPosition[0]</td>
    ///       <td>TriggerPosition[1]</td>
    ///       <td>0x00</td>
    ///       <td>0x00</td>
    ///       <td>TriggerPosition[2]</td>
    ///       <td>0x00</td>
    ///     </tr>
    ///   </table>
    /// </p>
    /// <p>
    ///   The samplerate is set relative to the base samplerate by a divider or to
    ///   a maximum samplerate.<br />
    ///   This divider is set by Tsr1Bits.samplerateId for values up to 5 with to
    ///   the following values:
    ///   <table>
    ///     <tr>
    ///       <td><b>Tsr1Bits.samplerateId</b></td><td>0</td><td>1</td><td>2</td><td>3</td>
    ///     </tr>
    ///     <tr>
    ///       <td><b>Samplerate</b></td><td>Max</td><td>Base</td><td>Base /
    ///       2</td><td>Base / 5</td>
    ///     </tr>
    ///   </table>
    ///   For higher divider values, the value can be set using the 16-bit value
    ///   in the two Downsampler bytes. The value of Downsampler is given by:<br
    ///   />
    ///   <i>Downsampler = 1comp((Base / Samplerate / 2) - 2)</i><br />
    ///   The Base samplerate is 50 MS/s for the DSO-2090 and DSO-2150. The Max
    ///   samplerate is also 50 MS/s for the DSO-2090 and 75 MS/s for the
    ///   DSO-2150.<br />
    ///   When using fast rate mode the Base and Max samplerate is twice as fast.
    ///   When Tsr1Bits.recordLength is 0 (Roll mode) the sampling rate is divided
    ///   by 1000.
    /// </p>
    /// <p>
    ///   The TriggerPosition sets the position of the pretrigger in samples. The
    ///   left side (0 %) is 0x77660 when using the small buffer and 0x78000 when
    ///   using the large buffer.
    /// </p>
    /// <p><br /></p>
    BULK_SETTRIGGERANDSAMPLERATE,

    /// BulkForceTrigger [<em>::MODEL_DSO2090, ::MODEL_DSO2150, ::MODEL_DSO2250,
    /// ::MODEL_DSO5200, ::MODEL_DSO5200A</em>]
    /// <p>
    ///   This command forces triggering:
    ///   <table>
    ///     <tr>
    ///       <td>0x02</td>
    ///       <td>0x00</td>
    ///     </tr>
    ///   </table>
    /// </p>
    /// <p><br /></p>
    BULK_FORCETRIGGER,

    /// BulkCaptureStart [<em>::MODEL_DSO2090, ::MODEL_DSO2150, ::MODEL_DSO2250,
    /// ::MODEL_DSO5200, ::MODEL_DSO5200A</em>]
    /// <p>
    ///   This command starts to capture data:
    ///   <table>
    ///     <tr>
    ///       <td>0x03</td>
    ///       <td>0x00</td>
    ///     </tr>
    ///   </table>
    /// </p>
    /// <p><br /></p>
    BULK_STARTSAMPLING,

    /// BulkTriggerEnabled [<em>::MODEL_DSO2090, ::MODEL_DSO2150, ::MODEL_DSO2250,
    /// ::MODEL_DSO5200, ::MODEL_DSO5200A</em>]
    /// <p>
    ///   This command sets the trigger:
    ///   <table>
    ///     <tr>
    ///       <td>0x04</td>
    ///       <td>0x00</td>
    ///     </tr>
    ///   </table>
    /// </p>
    /// <p><br /></p>
    BULK_ENABLETRIGGER,

    /// BulkGetData [<em>::MODEL_DSO2090, ::MODEL_DSO2150, ::MODEL_DSO2250,
    /// ::MODEL_DSO5200, ::MODEL_DSO5200A</em>]
    /// <p>
    ///   This command reads data from the hardware:
    ///   <table>
    ///     <tr>
    ///       <td>0x05</td>
    ///       <td>0x00</td>
    ///     </tr>
    ///   </table>
    /// </p>
    /// <p>
    ///   The oscilloscope returns the sample data, that will be split if it's
    ///   larger than the IN endpoint packet length:
    ///   <table>
    ///     <tr>
    ///       <td>Sample[0]</td>
    ///       <td>...</td>
    ///       <td>Sample[511]</td>
    ///     </tr>
    ///     <tr>
    ///       <td>Sample[512]</td>
    ///       <td>...</td>
    ///       <td>Sample[1023]</td>
    ///     </tr>
    ///     <tr>
    ///       <td>Sample[1024]</td>
    ///       <td colspan="2">...</td>
    ///     </tr>
    ///   </table>
    ///   Because of the 10 bit data model, the DSO-5200 transmits the two extra
    ///   bits for each sample afterwards:
    ///   <table>
    ///     <tr>
    ///       <td>Extra[0] << 2 | Extra[1]</td>
    ///       <td>0</td>
    ///       <td>Extra[2] << 2 | Extra[3]</td>
    ///       <td>0</td>
    ///       <td>...</td>
    ///       <td>Extra[510] << 2 | Extra[511]</td>
    ///       <td>0</td>
    ///     </tr>
    ///     <tr>
    ///       <td>Extra[512] << 2 | Extra[513]</td>
    ///       <td colspan="6">...</td>
    ///     </tr>
    ///   </table>
    /// </p>
    /// <p><br /></p>
    BULK_GETDATA,

    /// BulkGetCaptureState [<em>::MODEL_DSO2090, ::MODEL_DSO2150,
    /// ::MODEL_DSO2250, ::MODEL_DSO5200, ::MODEL_DSO5200A</em>]
    /// <p>
    ///   This command checks the capture state:
    ///   <table>
    ///     <tr>
    ///       <td>0x06</td>
    ///       <td>0x00</td>
    ///     </tr>
    ///   </table>
    /// </p>
    /// <p>
    ///   The oscilloscope returns it's capture state and the trigger point. Not
    ///   sure about this, looks like 248 16-bit words with nearly constant
    ///   values. These can be converted to the start address of the data in the
    ///   buffer (See Hantek::Control::calculateTriggerPoint):
    ///   <table>
    ///     <tr>
    ///       <td>::CaptureState</td>
    ///       <td>0x00</td>
    ///       <td>TriggerPoint[0]</td>
    ///       <td>TriggerPoint[1]</td>
    ///       <td>...</td>
    ///     </tr>
    ///   </table>
    /// </p>
    /// <p><br /></p>
    BULK_GETCAPTURESTATE,

    /// BulkSetGain [<em>::MODEL_DSO2090, ::MODEL_DSO2150, ::MODEL_DSO2250,
    /// ::MODEL_DSO5200, ::MODEL_DSO5200A</em>]
    /// <p>
    ///   This command sets the gain:
    ///   <table>
    ///     <tr>
    ///       <td>0x07</td>
    ///       <td>0x00</td>
    ///       <td>GainBits</td>
    ///       <td>0x00</td>
    ///       <td>0x00</td>
    ///       <td>0x00</td>
    ///       <td>0x00</td>
    ///       <td>0x00</td>
    ///     </tr>
    ///   </table>
    ///   It is usually used in combination with ::CONTROL_SETRELAYS.
    /// </p>
    /// <p><br /></p>
    BULK_SETGAIN,

    /// BulkSetLogicalData [<em></em>]
    /// <p>
    ///   This command sets the logical data (Not used in official %Hantek
    ///   software):
    ///   <table>
    ///     <tr>
    ///       <td>0x08</td>
    ///       <td>0x00</td>
    ///       <td>Data | 0x01</td>
    ///       <td>0x00</td>
    ///       <td>0x00</td>
    ///       <td>0x00</td>
    ///       <td>0x00</td>
    ///       <td>0x00</td>
    ///     </tr>
    ///   </table>
    /// </p>
    /// <p><br /></p>
    BULK_SETLOGICALDATA,

    /// BulkGetLogicalData [<em></em>]
    /// <p>
    ///   This command reads the logical data (Not used in official %Hantek
    ///   software):
    ///   <table>
    ///     <tr>
    ///       <td>0x09</td>
    ///       <td>0x00</td>
    ///     </tr>
    ///   </table>
    /// </p>
    /// <p>
    ///   The oscilloscope returns the logical data, which contains valid data in
    ///   the first byte although it is 64 or 512 bytes long:
    ///   <table>
    ///     <tr>
    ///       <td>Data</td>
    ///       <td>...</td>
    ///     </tr>
    ///   </table>
    /// </p>
    /// <p><br /></p>
    BULK_GETLOGICALDATA,

    /// [<em></em>]
    /// <p>
    ///   This command isn't used for any supported model:
    ///   <table>
    ///     <tr>
    ///       <td>0x0a</td>
    ///       <td>...</td>
    ///     </tr>
    ///   </table>
    /// </p>
    /// <p><br /></p>
    BULK_AUNKNOWN,

    /// BulkSetChannels2250 [<em>::MODEL_DSO2250</em>]
    /// <p>
    ///   This command sets the activated channels for the DSO-2250:
    ///   <table>
    ///     <tr>
    ///       <td>0x0b</td>
    ///       <td>0x00</td>
    ///       <td>BUsedChannels</td>
    ///       <td>0x00</td>
    ///     </tr>
    ///   </table>
    /// </p>
    /// <p><br /></p>
    BULK_BSETCHANNELS,

    /// BulkSetTrigger2250 [<em>::MODEL_DSO2250</em>]
    /// <p>
    ///   This command sets the trigger source for the DSO-2250:
    ///   <table>
    ///     <tr>
    ///       <td>0x0c</td>
    ///       <td>0x00</td>
    ///       <td>CTriggerBits</td>
    ///       <td>0x00</td>
    ///       <td>0x00</td>
    ///       <td>0x00</td>
    ///       <td>0x00</td>
    ///       <td>0x00</td>
    ///     </tr>
    ///   </table>
    /// </p>
    /// <p><br /></p>
    /// BulkSetSamplerate5200 [<em>::MODEL_DSO5200, ::MODEL_DSO5200A</em>]
    /// <p>
    ///   This command sets the sampling rate for the DSO-5200:
    ///   <table>
    ///     <tr>
    ///       <td>0x0c</td>
    ///       <td>0x00</td>
    ///       <td>SamplerateSlow[0]</td>
    ///       <td>SamplerateSlow[1]</td>
    ///       <td>SamplerateFast</td>
    ///       <td>0x00</td>
    ///     </tr>
    ///   </table>
    /// </p>
    /// <p>
    ///   The samplerate is set relative to the maximum sample rate by a divider
    ///   that is set in SamplerateFast and the 16-bit value in the two
    ///   SamplerateSlow bytes.<br />
    ///   Without using fast rate mode, the samplerate is:<br />
    ///   <i>Samplerate = SamplerateMax / (2comp(SamplerateSlow) * 2 + 4 -
    ///   SamplerateFast)</i><br />
    ///   SamplerateBase is 100 MS/s for the DSO-5200 in normal mode and 200 MS/s
    ///   in fast rate mode, the modifications regarding record length are the the
    ///   same that apply for the DSO-2090. The maximum samplerate is 125 MS/s in
    ///   normal mode and 250 MS/s in fast rate mode, and is reached by setting
    ///   SamplerateSlow = 0 and SamplerateFast = 4.
    /// </p>
    /// <p><br /></p>
    BULK_CSETTRIGGERORSAMPLERATE,

    /// BulkSetRecordLength2250 [<em>::MODEL_DSO2250</em>]
    /// <p>
    ///   This command sets the record length for the DSO-2250:
    ///   <table>
    ///     <tr>
    ///       <td>0x0d</td>
    ///       <td>0x00</td>
    ///       <td>::RecordLengthId</td>
    ///       <td>0x00</td>
    ///     </tr>
    ///   </table>
    /// </p>
    /// <p><br /></p>
    /// BulkSetBuffer5200 [<em>::MODEL_DSO5200, ::MODEL_DSO5200A</em>]
    /// <p>
    ///   This command sets the trigger position and record length for the
    ///   DSO-5200:
    ///   <table>
    ///     <tr>
    ///       <td>0x0d</td>
    ///       <td>0x00</td>
    ///       <td>TriggerPositionPre[0]</td>
    ///       <td>TriggerPositionPre[1]</td>
    ///       <td>::DTriggerPositionUsed</td>
    ///     </tr>
    ///   </table>
    ///   <table>
    ///     <tr>
    ///       <td>0xff</td>
    ///       <td>TriggerPositionPost[0]</td>
    ///       <td>TriggerPositionPost[1]</td>
    ///       <td>DBufferBits</td>
    ///       <td>0xff</td>
    ///     </tr>
    ///   </table>
    /// </p>
    /// <p>
    ///   The TriggerPositionPre and TriggerPositionPost values set the pretrigger
    ///   position. Both values have a range from 0xd7ff (0xc7ff for 14 kiS
    ///   buffer) to 0xfffe. On the left side (0 %) the TriggerPositionPre value
    ///   is minimal, on the right side (100 %) it is maximal. The
    ///   TriggerPositionPost value is maximal for 0 % and minimal for 100%.
    /// </p>
    /// <p><br /></p>
    BULK_DSETBUFFER,

    /// BulkSetSamplerate2250 [<em>::MODEL_DSO2250</em>]
    /// <p>
    ///   This command sets the samplerate:
    ///   <table>
    ///     <tr>
    ///       <td>0x0e</td>
    ///       <td>0x00</td>
    ///       <td>ESamplerateBits</td>
    ///       <td>0x00</td>
    ///       <td>Samplerate[0]</td>
    ///       <td>Samplerate[1]</td>
    ///       <td>0x00</td>
    ///       <td>0x00</td>
    ///     </tr>
    ///   </table>
    /// </p>
    /// <p>
    ///   The downsampler can be activated by setting ESamplerateBits.downsampling
    ///   = 1. If this is the case, the value of Downsampler is given by:<br />
    ///   <i>Downsampler = 1comp((Base / Samplerate) - 2)</i><br />
    ///   Base is 100 MS/s for the DSO-2250 in standard mode and 200 MS/s in fast
    ///   rate mode, the modifications regarding record length are the the same
    ///   that apply for the DSO-2090. The maximum samplerate is 125 MS/s in
    ///   standard mode and 250 MS/s in fast rate mode and is achieved by setting
    ///   ESamplerateBits.downsampling = 0.
    /// </p>
    /// <p><br /></p>
    /// BulkSetTrigger5200 [<em>::MODEL_DSO5200, ::MODEL_DSO5200A</em>]
    /// <p>
    ///   This command sets the channel and trigger settings:
    ///   <table>
    ///     <tr>
    ///       <td>0x0e</td>
    ///       <td>0x00</td>
    ///       <td>ETsrBits</td>
    ///       <td>0x00</td>
    ///       <td>0x00</td>
    ///       <td>0x00</td>
    ///       <td>0x00</td>
    ///       <td>0x00</td>
    ///     </tr>
    ///   </table>
    /// </p>
    /// <p><br /></p>
    BULK_ESETTRIGGERORSAMPLERATE,

    /// BulkSetBuffer2250 [<em>::MODEL_DSO2250</em>]
    /// <p>
    ///   This command sets the trigger position and buffer configuration for the
    ///   DSO-2250:
    ///   <table>
    ///     <tr>
    ///       <td>0x0f</td>
    ///       <td>0x00</td>
    ///       <td>TriggerPositionPost[0]</td>
    ///       <td>TriggerPositionPost[1]</td>
    ///       <td>TriggerPositionPost[2]</td>
    ///       <td>0x00</td>
    ///     </tr>
    ///   </table>
    ///   <table>
    ///     <tr>
    ///       <td>TriggerPositionPre[0]</td>
    ///       <td>TriggerPositionPre[1]</td>
    ///       <td>TriggerPositionPre[2]</td>
    ///       <td>0x00</td>
    ///       <td>0x00</td>
    ///       <td>0x00</td>
    ///     </tr>
    ///   </table>
    /// </p>
    /// <p>
    ///   The TriggerPositionPre and TriggerPositionPost values set the pretrigger
    ///   position. Both values have a range from 0x7d800 (0x00000 for 512 kiS
    ///   buffer) to 0x7ffff. On the left side (0 %) the TriggerPositionPre value
    ///   is minimal, on the right side (100 %) it is maximal. The
    ///   TriggerPositionPost value is maximal for 0 % and minimal for 100%.
    /// </p>
    /// <p><br /></p>
    BULK_FSETBUFFER,

    BULK_COUNT
};

}
