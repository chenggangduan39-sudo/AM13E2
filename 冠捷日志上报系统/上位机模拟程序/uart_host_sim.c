#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define REQUEST_FRAME_HEADER_0 0x81
#define REQUEST_FRAME_HEADER_1 0xAB
#define RESPONSE_FRAME_HEADER_0 0x90
#define RESPONSE_FRAME_HEADER_1 0x40
#define FRAME_FOOTER 0xFF

static uint16_t calculate_modbus_crc(const uint8_t *data, size_t len)
{
	uint16_t crc = 0xFFFF;
	for (size_t i = 0; i < len; ++i)
	{
		crc ^= data[i];
		for (int j = 0; j < 8; ++j)
		{
			if (crc & 0x0001)
			{
				crc >>= 1;
				crc ^= 0xA001;
			}
			else
			{
				crc >>= 1;
			}
		}
	}
	return crc;
}

static void emit_ack(uint16_t base_code, uint8_t status)
{
	uint16_t ack_code = base_code | 0x8000;
	uint8_t event_le[2] = {(uint8_t)(ack_code & 0xFF), (uint8_t)(ack_code >> 8)};
	uint16_t data_len = 1;
	uint8_t len_le[2] = {(uint8_t)(data_len & 0xFF), (uint8_t)(data_len >> 8)};
	uint8_t payload = status;
	uint8_t crc_buf[4 + 1];
	memcpy(crc_buf, event_le, 2);
	memcpy(crc_buf + 2, len_le, 2);
	crc_buf[4] = payload;
	uint16_t crc = calculate_modbus_crc(crc_buf, sizeof(crc_buf));
	uint8_t frame[2 + 2 + 2 + 1 + 2 + 1];
	size_t pos = 0;
	frame[pos++] = REQUEST_FRAME_HEADER_0;
	frame[pos++] = REQUEST_FRAME_HEADER_1;
	memcpy(frame + pos, event_le, 2);
	pos += 2;
	memcpy(frame + pos, len_le, 2);
	pos += 2;
	frame[pos++] = payload;
	frame[pos++] = (uint8_t)(crc & 0xFF);
	frame[pos++] = (uint8_t)(crc >> 8);
	frame[pos++] = FRAME_FOOTER;
	fwrite(frame, 1, pos, stdout);
	fflush(stdout);
	fprintf(stderr, "[HOST] ACK 0x%04X status=0x%02X\n", ack_code, status);
}

int main(int argc, char **argv)
{
	int verbose = 0;
	for (int i = 1; i < argc; ++i)
	{
		if (strcmp(argv[i], "--verbose") == 0)
		{
			verbose = 1;
		}
		else
		{
			fprintf(stderr, "Usage: %s [--verbose] < input_frames.bin > output_ack.bin\n", argv[0]);
			return 1;
		}
	}

	uint8_t *buffer = NULL;
	size_t buffer_len = 0;
	size_t buffer_cap = 0;
	uint8_t chunk[4096];
	size_t n;
	while ((n = fread(chunk, 1, sizeof(chunk), stdin)) > 0)
	{
		if (buffer_len + n > buffer_cap)
		{
			size_t new_cap = buffer_cap == 0 ? 8192 : buffer_cap * 2;
			while (new_cap < buffer_len + n)
			{
				new_cap *= 2;
			}
			uint8_t *tmp = (uint8_t *)realloc(buffer, new_cap);
			if (!tmp)
			{
				fprintf(stderr, "OOM while buffering input\n");
				free(buffer);
				return 1;
			}
			buffer = tmp;
			buffer_cap = new_cap;
		}
		memcpy(buffer + buffer_len, chunk, n);
		buffer_len += n;
	}

	size_t idx = 0;
	while (idx + 9 <= buffer_len)
	{
		if (buffer[idx] != RESPONSE_FRAME_HEADER_0 || buffer[idx + 1] != RESPONSE_FRAME_HEADER_1)
		{
			idx++;
			continue;
		}
		uint16_t event_code = (uint16_t)buffer[idx + 2] | ((uint16_t)buffer[idx + 3] << 8);
		uint16_t data_len = (uint16_t)buffer[idx + 4] | ((uint16_t)buffer[idx + 5] << 8);
		size_t frame_len = 2 + 2 + 2 + data_len + 2 + 1;
		if (idx + frame_len > buffer_len)
		{
			break;
		}
		if (buffer[idx + frame_len - 1] != FRAME_FOOTER)
		{
			idx++;
			continue;
		}
		uint16_t recv_crc = (uint16_t)buffer[idx + 6 + data_len] | ((uint16_t)buffer[idx + 6 + data_len + 1] << 8);
		uint16_t calc_crc = calculate_modbus_crc(&buffer[idx + 2], 4 + data_len);
		if (recv_crc != calc_crc)
		{
			fprintf(stderr, "[HOST] CRC mismatch for 0x%04X (expected 0x%04X got 0x%04X)\n",
					event_code, calc_crc, recv_crc);
			idx += frame_len;
			continue;
		}
		if (verbose)
		{
			fprintf(stderr, "[HOST] frame evt=0x%04X len=%u\n", event_code, data_len);
		}
		if (event_code == 0x0120 || event_code == 0x0122)
		{
			emit_ack(event_code, 0x00);
		}
		idx += frame_len;
	}
	free(buffer);
	return 0;
}
