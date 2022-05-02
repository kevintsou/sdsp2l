#pragma once

#define PATTERN_TOKEN_ADDR						(0xAB)
#define PATTERN_TOKEN_CMD						(0xC2)
#define PATTERN_TOKEN_READ_DATA_WITH_KBYTES     (0xDA)
#define PATTERN_TOKEN_READ_DATA_WITH_BYTES      (0xDB)
#define PATTERN_TOKEN_WRITE_DATA                (0xDC)

typedef enum
{
	FLASH_MODE_READ		= 0x00,
	FLASH_MODE_ERASE	= 0x60,
	FLASH_MODE_PROGRAM	= 0x80,
	FLASH_MODE_STANDBY	= 0xfe,
	FLASH_MODE_RESET	= 0xff,
} FLASH_MODE;

typedef struct s_virtual_dev_addr
{
	int ch;
	int block;
	int plane;
	int page;
}t_virtual_dev_addr;

typedef struct s_pattern_header
{
	unsigned char token;
	unsigned char lengthOfDescription;
	unsigned char param;
}t_pattern_header;

typedef union
{
	unsigned short all;
	struct
	{
		unsigned char lowByte;
		unsigned char highByte;
	} b;
} t_block, t_page;

typedef	union
{
	struct s_raw_packed
	{
		unsigned char token;
		unsigned char data[6];
	} raw;

	struct s_token_addr_packed
	{
		unsigned char token;
		unsigned char val;
		unsigned char resvered[5];
	} addr;

	struct s_token_full_addr_packed
	{
		unsigned char token;
		unsigned char _1stCol;
		unsigned char _2ndCol;
		unsigned char _1stRow;
		unsigned char _2ndRow;
		unsigned char _3rdRow;
		unsigned char _4thRow;
	} full_addr;

	struct s_token_cmd_packed
	{
		unsigned char token;
		unsigned char val;
		unsigned char uresvered[5];
	} cmd;

	struct s_token_read_data_packed
	{
		unsigned char   token;
		unsigned char   size;
		t_block			block;
		t_page			page;
		unsigned char	resvered[1];
	} data;

	struct s_delay_packed
	{
		unsigned char token;
		unsigned char time;
		unsigned char resvered[5];
	} delay;
} t_pattern_unit;


