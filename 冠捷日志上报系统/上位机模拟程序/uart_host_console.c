#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

#define REQUEST_HEADER0 0x81
#define REQUEST_HEADER1 0xAB
#define RESPONSE_HEADER0 0x90
#define RESPONSE_HEADER1 0x40
#define FRAME_FOOTER 0xFF

#define EVT_LOG_UPLOAD_START 0x0120
#define EVT_LOG_UPLOAD_DATA 0x0121
#define EVT_LOG_UPLOAD_END 0x0122
#define EVT_LOG_UPLOAD_START_ACK 0x8120
#define EVT_LOG_UPLOAD_END_ACK 0x8122
#define EVT_LOG_COLLECTION 0x0119

static struct termios stdin_old;
static int stdin_raw_enabled = 0;
static volatile sig_atomic_t g_stop = 0;

static void handle_sigint(int sig)
{
	(void)sig;
	g_stop = 1;
}

static void restore_stdin(void)
{
	if (stdin_raw_enabled)
	{
		tcsetattr(STDIN_FILENO, TCSANOW, &stdin_old);
		stdin_raw_enabled = 0;
	}
}

static void set_stdin_raw(void)
{
	struct termios raw;
	if (tcgetattr(STDIN_FILENO, &stdin_old) != 0)
	{
		perror("tcgetattr(stdin)");
		return;
	}
	raw = stdin_old;
	cfmakeraw(&raw);
	if (tcsetattr(STDIN_FILENO, TCSANOW, &raw) != 0)
	{
		perror("tcsetattr(stdin)");
		return;
	}
	stdin_raw_enabled = 1;
}

static int configure_serial(int fd, int baudrate)
{
	struct termios tty;
	memset(&tty, 0, sizeof(tty));
	if (tcgetattr(fd, &tty) != 0)
	{
		perror("tcgetattr(serial)");
		return -1;
	}

	speed_t speed;
	switch (baudrate)
	{
	case 9600:
		speed = B9600;
		break;
	case 19200:
		speed = B19200;
		break;
	case 38400:
		speed = B38400;
		break;
	case 57600:
		speed = B57600;
		break;
	case 115200:
	default:
		speed = B115200;
		break;
	}

	cfsetospeed(&tty, speed);
	cfsetispeed(&tty, speed);

	tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS8;
	tty.c_iflag &= ~(IGNBRK | BRKINT | ICRNL | INLCR | PARMRK | IXON);
	tty.c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);
	tty.c_oflag &= ~OPOST;
	tty.c_cflag |= (CLOCAL | CREAD);
	tty.c_cflag &= ~(PARENB | PARODD | CSTOPB | CRTSCTS);
	tty.c_cc[VMIN] = 1;
	tty.c_cc[VTIME] = 0;

	if (tcsetattr(fd, TCSANOW, &tty) != 0)
	{
		perror("tcsetattr(serial)");
		return -1;
	}
	return 0;
}

static uint16_t calculate_crc(const uint8_t *data, size_t len)
{
	uint16_t crc = 0xFFFF;
	for (size_t i = 0; i < len; ++i)
	{
		crc ^= data[i];
		for (int j = 0; j < 8; ++j)
		{
			if (crc & 1)
			{
				crc = (crc >> 1) ^ 0xA001;
			}
			else
			{
				crc >>= 1;
			}
		}
	}
	return crc;
}

static int send_request_frame(int fd, uint16_t event, const uint8_t *payload, uint16_t payload_len)
{
	uint8_t header[2] = {REQUEST_HEADER0, REQUEST_HEADER1};
	uint8_t event_le[2] = {(uint8_t)(event & 0xFF), (uint8_t)(event >> 8)};
	uint8_t len_le[2] = {(uint8_t)(payload_len & 0xFF), (uint8_t)(payload_len >> 8)};
	uint8_t crc_buf[4 + payload_len];
	memcpy(crc_buf, event_le, 2);
	memcpy(crc_buf + 2, len_le, 2);
	if (payload_len && payload)
	{
		memcpy(crc_buf + 4, payload, payload_len);
	}
	uint16_t crc = calculate_crc(crc_buf, sizeof(crc_buf));

	uint8_t buf[512];
	size_t pos = 0;
	buf[pos++] = header[0];
	buf[pos++] = header[1];
	memcpy(buf + pos, event_le, 2);
	pos += 2;
	memcpy(buf + pos, len_le, 2);
	pos += 2;
	if (payload_len && payload)
	{
		memcpy(buf + pos, payload, payload_len);
		pos += payload_len;
	}
	buf[pos++] = (uint8_t)(crc & 0xFF);
	buf[pos++] = (uint8_t)(crc >> 8);
	buf[pos++] = FRAME_FOOTER;

	ssize_t n = write(fd, buf, pos);
	if (n != (ssize_t)pos)
	{
		perror("write(serial)");
		return -1;
	}
	return 0;
}

static int send_ack(int fd, uint16_t base_event, uint8_t status)
{
	uint16_t ack_event = EVT_LOG_UPLOAD_START;
	if (base_event == EVT_LOG_UPLOAD_END)
	{
		ack_event = EVT_LOG_UPLOAD_END_ACK;
	}
	else
	{
		ack_event = EVT_LOG_UPLOAD_START_ACK;
	}
	uint8_t payload[1] = {status};
	return send_request_frame(fd, ack_event, payload, sizeof(payload));
}

static int send_collect_request(int fd)
{
	return send_request_frame(fd, EVT_LOG_COLLECTION, NULL, 0);
}

static void usage(const char *prog)
{
	fprintf(stderr, "Usage: %s <serial_device> [baud]\n", prog);
	fprintf(stderr, "Commands: 'c' trigger log upload, 'q' quit.\n");
}

int main(int argc, char **argv)
{
	if (argc < 2)
	{
		usage(argv[0]);
		return 1;
	}
	const char *dev = argv[1];
	int baud = 115200;
	if (argc >= 3)
	{
		baud = atoi(argv[2]);
		if (baud <= 0)
			baud = 115200;
	}

	int fd = open(dev, O_RDWR | O_NOCTTY | O_SYNC);
	if (fd < 0)
	{
		perror("open serial");
		return 1;
	}
	if (configure_serial(fd, baud) != 0)
	{
		close(fd);
		return 1;
	}

	set_stdin_raw();
	atexit(restore_stdin);
	signal(SIGINT, handle_sigint);

	printf("UART host console started on %s @ %d baud.\n", dev, baud);
	printf("Press 'c' to send log-collection command, 'q' to quit.\n");

	uint8_t rx_buf[4096];
	uint8_t frame_buf[65536];
	size_t frame_len = 0;

	while (!g_stop)
	{
		struct pollfd fds[2];
		fds[0].fd = fd;
		fds[0].events = POLLIN;
		fds[1].fd = STDIN_FILENO;
		fds[1].events = POLLIN;
		int ret = poll(fds, 2, 200);
		if (ret < 0)
		{
			if (errno == EINTR)
				continue;
			perror("poll");
			break;
		}
		if (ret == 0)
		{
			continue;
		}
		if (fds[1].revents & POLLIN)
		{
			char ch;
			ssize_t n = read(STDIN_FILENO, &ch, 1);
			if (n > 0)
			{
				if (ch == 'c' || ch == 'C')
				{
					if (send_collect_request(fd) == 0)
					{
						printf("[HOST] sent 0x0119 request\n");
					}
					else
					{
						fprintf(stderr, "[HOST] send request failed\n");
					}
				}
				else if (ch == 'q' || ch == 'Q')
				{
					printf("[HOST] quit requested\n");
					break;
				}
			}
		}
		if (fds[0].revents & POLLIN)
		{
			ssize_t n = read(fd, rx_buf, sizeof(rx_buf));
			if (n > 0)
			{
				if (frame_len + (size_t)n > sizeof(frame_buf))
				{
					frame_len = 0;
				}
				memcpy(frame_buf + frame_len, rx_buf, n);
				frame_len += (size_t)n;

				size_t pos = 0;
				while (frame_len - pos >= 9)
				{
					if (frame_buf[pos] != RESPONSE_HEADER0 || frame_buf[pos + 1] != RESPONSE_HEADER1)
					{
						pos++;
						continue;
					}
					uint16_t evt = (uint16_t)frame_buf[pos + 2] | ((uint16_t)frame_buf[pos + 3] << 8);
					uint16_t data_len = (uint16_t)frame_buf[pos + 4] | ((uint16_t)frame_buf[pos + 5] << 8);
					size_t total = 2 + 2 + 2 + data_len + 2 + 1;
					if (frame_len - pos < total)
					{
						break;
					}
					if (frame_buf[pos + total - 1] != FRAME_FOOTER)
					{
						pos++;
						continue;
					}
					uint16_t crc_rx = (uint16_t)frame_buf[pos + 6 + data_len] |
									  ((uint16_t)frame_buf[pos + 6 + data_len + 1] << 8);
					uint16_t crc_calc = calculate_crc(frame_buf + pos + 2, 4 + data_len);
					if (crc_rx != crc_calc)
					{
						printf("[HOST] CRC mismatch for evt=0x%04X\n", evt);
						pos += total;
						continue;
					}
					printf("[HOST] frame evt=0x%04X len=%u\n", evt, data_len);
					if (evt == EVT_LOG_UPLOAD_START || evt == EVT_LOG_UPLOAD_END)
					{
						if (send_ack(fd, evt, 0x00) == 0)
						{
							printf("[HOST] acked 0x%04X\n", evt);
						}
						else
						{
							fprintf(stderr, "[HOST] failed to ack 0x%04X\n", evt);
						}
					}
					pos += total;
				}
				if (pos > 0)
				{
					memmove(frame_buf, frame_buf + pos, frame_len - pos);
					frame_len -= pos;
				}
			}
		}
	}

	restore_stdin();
	close(fd);
	printf("UART host console stopped.\n");
	return 0;
}
