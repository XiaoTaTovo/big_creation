#ifndef PD_CAPTURE_H
#define PD_CAPTURE_H

#include <stdint.h>

/* Application API version, not the USB PD revision. */
#define PD_CAPTURE_API_VERSION 1U
#define PD_CAPTURE_PAYLOAD_MAX 28U

#define PD_CHECK_UNKNOWN 0U
#define PD_CHECK_PASS    1U
#define PD_CHECK_FAIL    2U

#define PD_RECORD_FRAME       1U
#define PD_RECORD_HARD_RESET  2U
#define PD_RECORD_CABLE_RESET 3U
#define PD_RECORD_DETACH      4U

#define PD_CAPTURE_HAS_HEADER 0x01U
#define PD_CAPTURE_HAS_TIME   0x02U
#define PD_CAPTURE_TRUNCATED  0x04U

#define PD_SOP_UNKNOWN 0U
#define PD_SOP_NORMAL  1U
#define PD_SOP_PRIME   2U
#define PD_SOP_DPRIME  3U

/* Local API object only: NOT a wire format or shared-memory layout.
 * Zero-initialize before filling. No pointers or LVGL objects are transported.
 * Unsupported SOP types must remain UNKNOWN, never be relabeled NORMAL.
 */
typedef struct
{
    uint32_t sequence;       /* Capture sequence; advances even if submit fails. */
    uint32_t timestamp_us;   /* Frame start, us since V3 boot, wraps modulo 2^32.
                             * Meaningful only with HAS_TIME. */
    uint16_t header;         /* Complete header; valid only with HAS_HEADER. */
    uint8_t kind;
    uint8_t flags;
    uint8_t cc;              /* 0 unknown, 1 CC1, 2 CC2. */
    uint8_t sop;
    uint8_t crc_status;      /* UNKNOWN / PASS / FAIL; never assume PASS. */
    uint8_t length_status;   /* Captured frame check, not payload buffer length. */
    uint8_t payload_len;     /* Bytes retained, 0..PD_CAPTURE_PAYLOAD_MAX. */
    uint8_t reserved[3];     /* Must be zero. */
    /* Excludes header and CRC; ordinary data objects are little-endian.
     * Set TRUNCATED when bytes were omitted; unused bytes must be zero.
     * Reset/detach records have no header/payload; checks remain UNKNOWN.
     */
    uint8_t payload[PD_CAPTURE_PAYLOAD_MAX];
} PdCaptureRecord;

typedef enum
{
    PD_SUBMIT_OK = 0,
    PD_SUBMIT_FULL,
    PD_SUBMIT_INVALID,
    PD_SUBMIT_NOT_READY
} PdSubmitResult;

/* V3 main-context single producer only, never call from ISR.
 * OK means copied into shared queue; caller may reuse its record after return.
 * Full rejects newest record without waiting. Unknown/bad captured CRC is kept.
 * NOT_READY also covers latched transport faults; inspect shared diagnostics.
 */
PdSubmitResult PD_Capture_Submit(const PdCaptureRecord *record);

#endif /* PD_CAPTURE_H */
