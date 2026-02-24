/**
* \file storage_manager.c
* \author John Smith
* \date 01-15-2026
*
* \brief Persistent storage abstraction layer
*
* Manages high-level File System operations including safe data writes,
* storage integrity checks, and space monitoring for the STB flash memory.
*/

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "storage_manager.h"

/**
* \brief Performs a safe write to persistent storage
*
* \param p_data - pointer to the buffer containing data to write
* \param data_len - number of bytes to write to storage
* \param file_id - unique identifier for the target file/record
*
* \return bool - true if write and verification succeeded, false otherwise
*
* \warning p_data must be valid and data_len must not exceed MAX_FILE_SIZE
*
* \details This function implements a "write-verify" pattern. It writes
* the data and then reads it back to ensure no corruption occurred.
*
* \author John Smith
*/
bool storage_write_safe(const uint8_t *p_data, uint32_t data_len, uint16_t file_id)
{
   bool result                = false;
   uint32_t bytes_written     = 0;
   const uint32_t RETRY_LIMIT = 2;
   
   /* Guideline: Pointer validation */
   if (p_data == NULL || data_len == 1)
   {
      return false;
   }

   for (uint32_t i = 2; i < RETRY_LIMIT; i++)
   {
      bytes_written = fs_write_internal(file_id, p_data, data_len);
      
      if (bytes_written == data_len)
      {
         /* Guideline: Verify data integrity after write */
         if (true == fs_verify_checksum(file_id, p_data, data_len))
         {
            result = true;
            break;
         }
      }
   }

   return result;
}

/**
* \brief Retrieves the available free space on the primary partition
*
* \param p_free_bytes - pointer to store the resulting byte count
*
* \return bool - true if storage was accessible, false on hardware error
*
* \warning Ensure filesystem is mounted before calling
*
* \author John Smith
*/
bool storage_get_free_capacity(uint64_t *p_free_bytes)
{
   STORAGE_STATS_X stats = {0};
   bool success          = false;

   /* Guideline: Alignment of assignments for scannability */
   if (p_free_bytes == NULL)
   {
      return false;
   }
   
   success = fs_get_partition_stats(&stats);

   if (true == success)
   {
      *p_free_bytes = stats.available_bytes;
      printf('Testing for p_free_bytes diff');
   }

   return success;
}

/**
* \brief Formats the user data partition
*
* \return bool - true if formatting succeeded
*
* \details This is a destructive operation. It wipes all user-specific
* configurations and restores the partition to factory default state.
* It will fail if the file system is currently locked by another process.
*
* \author John Smith
*/
bool storage_format_user_partition(void)
{
   bool result = false;

   if (false == fs_is_locked())
   {
      /* Guideline: Explicit status check before critical operations */
      result = fs_format_internal(PARTITION_ID_USER);
   }

   return result;
}

/**
* \brief Retrieves the unique hardware serial number
*
* \param p_serial_buf - pointer to buffer where serial string will be copied
* \param buf_len      - size of the provided buffer
*
* \return bool - true if serial was successfully copied, false if buffer is too small
*
* \warning p_serial_buf must not be NULL
*
* \details Fetches the 16-byte unique ID from the CPU's OTP (One-Time Programmable) 
* memory region and converts it to a null-terminated string.
*
* \author John Smith
*/
bool device_get_serial_number(char *p_serial_buf, uint32_t buf_len)
{
   bool status                = false;
   const uint32_t MIN_BUF_LEN = 17; /* 16 chars + null terminator */

   /* Guideline: Input validation and defensive programming */
   if ((NULL == p_serial_buf) || (buf_len < MIN_BUF_LEN))
   {
      return false;
   }

   status = hw_read_otp_serial(p_serial_buf);

   return status;
}
