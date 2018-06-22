// Copyright (C) 2016  0CBH0 <maodatou88@163.com>
// Licensed under the terms of the GNU GPL, version 3
// http://www.gnu.org/licenses/gpl-3.0.txt

#include <io.h>
#include <string.h>
#include <vector>
#include <algorithm> 
#include <zlib.h>
#include "wiiurpxtool.h"

#pragma warning(disable : 4996)

using namespace std;

u16 Low2Big_u16(u16 a);
u32 Low2Big_u32(u32 a);
u64 Low2Big_u64(u64 a);
void fread16_BE(u16 &i, FILE *f);
void fread32_BE(u32 &i, FILE *f);
void fread64_BE(u64 &i, FILE *f);
void fwrite16_BE(u16 i, FILE *f);
void fwrite32_BE(u32 i, FILE *f);
void fwrite64_BE(u64 i, FILE *f);
u32 crc32_rpx(u32 crc, u8 *buff, u32 len);
bool SortFunc(const Elf32_Shdr_Sort &v1, const Elf32_Shdr_Sort &v2);
int fcopy(char *src_name, char *dest_name);
int decompress(FILE *in, FILE *out);
int compress(FILE *in, FILE *out);

int decompress(FILE *in, FILE *out)
{
	Elf32_Ehdr ehdr;
	for(u32 i = 0; i < 0x10; i++)
		ehdr.e_ident[i] = getc(in);
	tempa_u32 = ehdr.e_ident[0]<<24|ehdr.e_ident[1]<<16|ehdr.e_ident[2]<<8|ehdr.e_ident[3];
	if(tempa_u32!=0x7F454C46) return -1;
	fread16_BE(ehdr.e_type, in);
	if(ehdr.e_type != 0xFE01) return -1;
	fread16_BE(ehdr.e_machine, in);
	fread32_BE(ehdr.e_version, in);
	fread32_BE(ehdr.e_entry, in);
	fread32_BE(ehdr.e_phoff, in);
	fread32_BE(ehdr.e_shoff, in);
	fread32_BE(ehdr.e_flags, in);
	fread16_BE(ehdr.e_ehsize, in);
	fread16_BE(ehdr.e_phentsize, in);
	fread16_BE(ehdr.e_phnum, in);
	fread16_BE(ehdr.e_shentsize, in);
	fread16_BE(ehdr.e_shnum, in);
	fread16_BE(ehdr.e_shstrndx, in);
	ulg shdr_data_elf_offset = ehdr.e_shoff + ehdr.e_shnum * ehdr.e_shentsize;
	fwrite(ehdr.e_ident, sizeof(ehdr.e_ident), 1, out);
	fwrite16_BE(ehdr.e_type, out);
	fwrite16_BE(ehdr.e_machine, out);
	fwrite32_BE(ehdr.e_version, out);
	fwrite32_BE(ehdr.e_entry, out);
	fwrite32_BE(ehdr.e_phoff, out);
	fwrite32_BE(ehdr.e_shoff, out);
	fwrite32_BE(ehdr.e_flags, out);
	fwrite16_BE(ehdr.e_ehsize, out);
	fwrite16_BE(ehdr.e_phentsize, out);
	fwrite16_BE(ehdr.e_phnum, out);
	fwrite16_BE(ehdr.e_shentsize, out);
	fwrite16_BE(ehdr.e_shnum, out);
	fwrite16_BE(ehdr.e_shstrndx, out);
	fwrite32_BE(0x00000000, out);
	fwrite32_BE(0x00000000, out);
	fwrite32_BE(0x00000000, out);
	ulg crc_data_offset = 0; 
	u32 *crcs = new u32[ehdr.e_shnum];
	vector< Elf32_Shdr_Sort > shdr_table_index;
	Elf32_Shdr *shdr_table = new Elf32_Shdr[ehdr.e_shnum];
	while(ftell(out)<shdr_data_elf_offset) putc(0, out);
	fseek(in, ehdr.e_shoff, 0);
	for (u32 i=0; i<ehdr.e_shnum; i++)
	{
		crcs[i] = 0;
		fread32_BE(shdr_table[i].sh_name, in);
		fread32_BE(shdr_table[i].sh_type, in);
		fread32_BE(shdr_table[i].sh_flags, in);
		fread32_BE(shdr_table[i].sh_addr, in);
		fread32_BE(shdr_table[i].sh_offset, in);
		fread32_BE(shdr_table[i].sh_size, in);
		fread32_BE(shdr_table[i].sh_link, in);
		fread32_BE(shdr_table[i].sh_info, in);
		fread32_BE(shdr_table[i].sh_addralign, in);
		fread32_BE(shdr_table[i].sh_entsize, in);
		if (shdr_table[i].sh_offset != 0)
		{
			Elf32_Shdr_Sort shdr_index;
			shdr_index.index = i;
			shdr_index.sh_offset = shdr_table[i].sh_offset;
			shdr_table_index.push_back(shdr_index);
		}
	}
	sort(shdr_table_index.begin(), shdr_table_index.end(), SortFunc);
	for(vector<Elf32_Shdr_Sort>::iterator shdr_index=shdr_table_index.begin();shdr_index!=shdr_table_index.end();shdr_index++)
	{
		pos = shdr_table[shdr_index->index].sh_offset; fseek(in, pos, 0);
		shdr_table[shdr_index->index].sh_offset = ftell(out);
		if((shdr_table[shdr_index->index].sh_flags & SHF_RPL_ZLIB) == SHF_RPL_ZLIB)
		{
			u32 data_size = shdr_table[shdr_index->index].sh_size-4;
			fread32_BE(shdr_table[shdr_index->index].sh_size, in);
			u32 block_size = CHUNK;
			u32 have;
			z_stream strm;
			u8 buff_in[CHUNK];
			u8 buff_out[CHUNK];
			strm.zalloc = Z_NULL;
			strm.zfree = Z_NULL;
			strm.opaque = Z_NULL;
			strm.avail_in = 0;
			strm.next_in = Z_NULL;
			inflateInit(&strm);
			while(data_size>0)
			{
				block_size = CHUNK;
				if(data_size<block_size)
					block_size = data_size;
				data_size -= block_size;
				strm.avail_in = fread(buff_in, 1, block_size, in);
				strm.next_in = buff_in;
				do
				{
					strm.avail_out = CHUNK;
					strm.next_out = buff_out;
					inflate(&strm, Z_NO_FLUSH);
					have = CHUNK - strm.avail_out;
					fwrite(buff_out, 1, have, out);
					crcs[shdr_index->index] = crc32_rpx(crcs[shdr_index->index], buff_out, have);
				}while(strm.avail_out == 0);
			}
			inflateEnd(&strm);
			shdr_table[shdr_index->index].sh_flags &= ~SHF_RPL_ZLIB;
		}
		else
		{
			u32 data_size = shdr_table[shdr_index->index].sh_size;
			u32 block_size = CHUNK;
			while(data_size>0)
			{
				u8 data[CHUNK];
				block_size = CHUNK;
				if(data_size<block_size)
					block_size = data_size;
				data_size -= block_size;
				fread(data, 1, block_size, in);
				fwrite(data, 1, block_size, out);
				crcs[shdr_index->index] = crc32_rpx(crcs[shdr_index->index], data, block_size);
			}
		}
		while(ftell(out)%0x40!=0) putc(0, out);
		if((shdr_table[shdr_index->index].sh_type & SHT_RPL_CRCS) == SHT_RPL_CRCS)
		{
			crcs[shdr_index->index] = 0;
			crc_data_offset = shdr_table[shdr_index->index].sh_offset;
		}
	}
	fseek(out, ehdr.e_shoff, 0);
	for (u32 i=0; i<ehdr.e_shnum; i++)
	{
		fwrite32_BE(shdr_table[i].sh_name, out);
		fwrite32_BE(shdr_table[i].sh_type, out);
		fwrite32_BE(shdr_table[i].sh_flags, out);
		fwrite32_BE(shdr_table[i].sh_addr, out);
		fwrite32_BE(shdr_table[i].sh_offset, out);
		fwrite32_BE(shdr_table[i].sh_size, out);
		fwrite32_BE(shdr_table[i].sh_link, out);
		fwrite32_BE(shdr_table[i].sh_info, out);
		fwrite32_BE(shdr_table[i].sh_addralign, out);
		fwrite32_BE(shdr_table[i].sh_entsize, out);
	}
	
	fseek(out, crc_data_offset, 0);
	for (u32 i=0; i<ehdr.e_shnum; i++)
		fwrite32_BE(crcs[i], out);
	delete[]crcs;
	delete[]shdr_table;
	shdr_table_index.clear();
	return 0;
}

int compress(FILE *in, FILE *out)
{
	Elf32_Ehdr ehdr;
	for(u32 i = 0; i < 0x10; i++)
		ehdr.e_ident[i] = getc(in);
	tempa_u32 = ehdr.e_ident[0]<<24|ehdr.e_ident[1]<<16|ehdr.e_ident[2]<<8|ehdr.e_ident[3];
	if(tempa_u32!=0x7F454C46) return -1;
	fread16_BE(ehdr.e_type, in);
	if(ehdr.e_type != 0xFE01) return -1;
	fread16_BE(ehdr.e_machine, in);
	fread32_BE(ehdr.e_version, in);
	fread32_BE(ehdr.e_entry, in);
	fread32_BE(ehdr.e_phoff, in);
	fread32_BE(ehdr.e_shoff, in);
	fread32_BE(ehdr.e_flags, in);
	fread16_BE(ehdr.e_ehsize, in);
	fread16_BE(ehdr.e_phentsize, in);
	fread16_BE(ehdr.e_phnum, in);
	fread16_BE(ehdr.e_shentsize, in);
	fread16_BE(ehdr.e_shnum, in);
	fread16_BE(ehdr.e_shstrndx, in);
	ulg shdr_data_elf_offset = ehdr.e_shoff + ehdr.e_shnum * ehdr.e_shentsize;
	fwrite(ehdr.e_ident, sizeof(ehdr.e_ident), 1, out);
	fwrite16_BE(ehdr.e_type, out);
	fwrite16_BE(ehdr.e_machine, out);
	fwrite32_BE(ehdr.e_version, out);
	fwrite32_BE(ehdr.e_entry, out);
	fwrite32_BE(ehdr.e_phoff, out);
	fwrite32_BE(ehdr.e_shoff, out);
	fwrite32_BE(ehdr.e_flags, out);
	fwrite16_BE(ehdr.e_ehsize, out);
	fwrite16_BE(ehdr.e_phentsize, out);
	fwrite16_BE(ehdr.e_phnum, out);
	fwrite16_BE(ehdr.e_shentsize, out);
	fwrite16_BE(ehdr.e_shnum, out);
	fwrite16_BE(ehdr.e_shstrndx, out);
	fwrite32_BE(0x00000000, out);
	fwrite32_BE(0x00000000, out);
	fwrite32_BE(0x00000000, out);
	ulg crc_data_offset = 0; 
	u32 *crcs = new u32[ehdr.e_shnum];
	vector< Elf32_Shdr_Sort > shdr_table_index;
	Elf32_Shdr *shdr_table = new Elf32_Shdr[ehdr.e_shnum];
	while(ftell(out)<shdr_data_elf_offset) putc(0, out);
	fseek(in, ehdr.e_shoff, 0);
	for (u32 i=0; i<ehdr.e_shnum; i++)
	{
		crcs[i] = 0;
		fread32_BE(shdr_table[i].sh_name, in);
		fread32_BE(shdr_table[i].sh_type, in);
		fread32_BE(shdr_table[i].sh_flags, in);
		fread32_BE(shdr_table[i].sh_addr, in);
		fread32_BE(shdr_table[i].sh_offset, in);
		fread32_BE(shdr_table[i].sh_size, in);
		fread32_BE(shdr_table[i].sh_link, in);
		fread32_BE(shdr_table[i].sh_info, in);
		fread32_BE(shdr_table[i].sh_addralign, in);
		fread32_BE(shdr_table[i].sh_entsize, in);
		if (shdr_table[i].sh_offset != 0)
		{
			Elf32_Shdr_Sort shdr_index;
			shdr_index.index = i;
			shdr_index.sh_offset = shdr_table[i].sh_offset;
			shdr_table_index.push_back(shdr_index);
		}
	}
	sort(shdr_table_index.begin(), shdr_table_index.end(), SortFunc);
	for(vector<Elf32_Shdr_Sort>::iterator shdr_index=shdr_table_index.begin();shdr_index!=shdr_table_index.end();shdr_index++)
	{
		pos = shdr_table[shdr_index->index].sh_offset; fseek(in, pos, 0);
		shdr_table[shdr_index->index].sh_offset = ftell(out);
		if (((shdr_table[shdr_index->index].sh_type & SHT_RPL_FILEINFO) == SHT_RPL_FILEINFO)||
			((shdr_table[shdr_index->index].sh_type & SHT_RPL_CRCS) == SHT_RPL_CRCS)||
			((shdr_table[shdr_index->index].sh_flags & SHF_RPL_ZLIB) == SHF_RPL_ZLIB))
		{
			u32 data_size = shdr_table[shdr_index->index].sh_size;
			u32 block_size = CHUNK;
			while(data_size>0)
			{
				u8 data[CHUNK];
				block_size = CHUNK;
				if(data_size<block_size)
					block_size = data_size;
				data_size -= block_size;
				fread(data, 1, block_size, in);
				fwrite(data, 1, block_size, out);
				crcs[shdr_index->index] = crc32_rpx(crcs[shdr_index->index], data, block_size);
			}
		}
		else
		{
			u32 data_size = shdr_table[shdr_index->index].sh_size;
			u32 block_size = CHUNK;
			u32 have;
			z_stream strm;
			u8 buff_in[CHUNK];
			u8 buff_out[CHUNK];
			strm.zalloc = Z_NULL;
			strm.zfree = Z_NULL;
			strm.opaque = Z_NULL;
			strm.avail_in = 0;
			strm.next_in = Z_NULL;
			if(data_size<CHUNK)
			{
				block_size = data_size;
				deflateInit(&strm, LEVEL);
				strm.avail_in = fread(buff_in, 1, block_size, in);
				crcs[shdr_index->index] = crc32_rpx(crcs[shdr_index->index], buff_in, block_size);
				strm.next_in = buff_in;
				strm.avail_out = CHUNK;
				strm.next_out = buff_out;
				deflate(&strm, Z_FINISH);
				have = CHUNK - strm.avail_out;
				if(have+4 < block_size)
				{
					fwrite32_BE(data_size, out);
					fwrite(buff_out, 1, have, out);
					shdr_table[shdr_index->index].sh_size = have+4;
					shdr_table[shdr_index->index].sh_flags |= SHF_RPL_ZLIB;
				}
				else
					fwrite(buff_in, 1, block_size, out);
				deflateEnd(&strm);
			}
			else
			{
				s32 flush = Z_NO_FLUSH;
				u32 compress_size = 4;
				fwrite32_BE(data_size, out);
				deflateInit(&strm, LEVEL);
				while(data_size>0)
				{
					block_size = CHUNK;
					flush = Z_NO_FLUSH;
					if(data_size <= block_size)
					{
						block_size = data_size;
						flush = Z_FINISH;
					}
					data_size -= block_size;
					strm.avail_in = fread(buff_in, 1, block_size, in);
					crcs[shdr_index->index] = crc32_rpx(crcs[shdr_index->index], buff_in, block_size);
					strm.next_in = buff_in;
					do{
						strm.avail_out = CHUNK;
						strm.next_out = buff_out;
						deflate(&strm, flush);
						have = CHUNK - strm.avail_out;
						fwrite(buff_out, 1, have, out);
						compress_size += have;
					}while(strm.avail_out == 0);
				}
				deflateEnd(&strm);
				shdr_table[shdr_index->index].sh_size = compress_size;
				shdr_table[shdr_index->index].sh_flags |= SHF_RPL_ZLIB;
			}
		}
		while(ftell(out)%0x40!=0) putc(0, out);
		if((shdr_table[shdr_index->index].sh_type & SHT_RPL_CRCS) == SHT_RPL_CRCS)
		{
			crcs[shdr_index->index] = 0;
			crc_data_offset = shdr_table[shdr_index->index].sh_offset;
		}
	}
	fseek(out, ehdr.e_shoff, 0);
	for (u32 i=0; i<ehdr.e_shnum; i++)
	{
		fwrite32_BE(shdr_table[i].sh_name, out);
		fwrite32_BE(shdr_table[i].sh_type, out);
		fwrite32_BE(shdr_table[i].sh_flags, out);
		fwrite32_BE(shdr_table[i].sh_addr, out);
		fwrite32_BE(shdr_table[i].sh_offset, out);
		fwrite32_BE(shdr_table[i].sh_size, out);
		fwrite32_BE(shdr_table[i].sh_link, out);
		fwrite32_BE(shdr_table[i].sh_info, out);
		fwrite32_BE(shdr_table[i].sh_addralign, out);
		fwrite32_BE(shdr_table[i].sh_entsize, out);
	}

	fseek(out, crc_data_offset, 0);
	for (u32 i=0; i<ehdr.e_shnum; i++)
		fwrite32_BE(crcs[i], out);
	delete[]crcs;
	delete[]shdr_table;
	shdr_table_index.clear();
	return 0;
}

int main(int argc, char *argv[])
{
	if(argc < 3)
	{
		printf("wiiurpxtool - version:1.3\n");
		printf("Compress or decompress RPL/RPX files for Wii U\n\n");
		printf("Usage:\n");
		printf("decompress:\n");
		printf("wiiurpxtool -d <rpx_name> [out_name]\n");
		printf("compress:\n");
		printf("wiiurpxtool -c <rpx_name> [out_name]\n");
		return 0;
	}
	FILE *in = fopen(argv[2], "rb");
	if(in == NULL) return -1;
	FILE *out;
	if(argc == 3)
		out = fopen("temp.bin", "wb");
	else
	{
		out = fopen(argv[3], "wb");
		if (out == NULL) return -1;
	}
	s32 result = -1;
	if(strcmp("-d", argv[1]) == 0)
	{
		printf("decompressing...\n");
		result = decompress(in, out);
	}
	if(strcmp("-c", argv[1]) == 0)
	{
		printf("compressing...\n");
		result = compress(in, out);
	}
	fclose(in);
	fclose(out);
	if(argc == 3 && result == 0)
		fcopy("temp.bin", argv[2]);
	remove("temp.bin");
	return 0;
}

u32 crc32_rpx(u32 crc, u8 *buff, u32 len)
{
	u32 crc_table[256];
	for(u32 i=0; i<256; i++)
	{
		u32 c = i;
		for(u32 j=0; j<8; j++)
		{
			if(c & 1)
				c = 0xedb88320L^(c>>1);
			else
				c = c>>1;
		}
		crc_table[i] = c;
	}
    crc = ~crc;
	for(u32 i=0; i<len; i++)
        crc = (crc>>8)^crc_table[(crc^buff[i])&0xff];
    return ~crc;
}

bool SortFunc(const Elf32_Shdr_Sort &v1, const Elf32_Shdr_Sort &v2)
{
	//
	return v1.sh_offset < v2.sh_offset;
}

u16 Low2Big_u16(u16 a)
{
	u16 b=0;
	b=(a&0xFF00)>>8|(a&0xFF)<<8;
	return b;
}

u32 Low2Big_u32(u32 a)
{
	u32 b=0;
	b=(a&0xFF000000)>>24|(a&0xFF0000)>>8|(a&0xFF00)<<8|(a&0xFF)<<24;
	return b;
}

u64 Low2Big_u64(u64 a)
{
	u64 b=0;
	b = (a&0xFF)<<56|(a&0xFF00)<<40|(a&0xFF0000)<<24|(a&0xFF000000)<<8|(a&0xFF00000000)>>8|
		(a&0xFF0000000000)>>24|(a&0xFF000000000000)>>40|(a&0xFF00000000000000)>>56;
	return b;
}

void fread16_BE(u16 &i, FILE *f)
{
	u16 p = 0;
	fread(&p, sizeof(u16), 1, f);
	i = Low2Big_u16(p);
}

void fread32_BE(u32 &i, FILE *f)
{
	u32 p = 0;
	fread(&p, sizeof(u32), 1, f);
	i = Low2Big_u32(p);
}

void fread64_BE(u64 &i, FILE *f)
{
	u64 p = 0;
	fread(&p, sizeof(u64), 1, f);
	i = Low2Big_u64(p);
}

void fwrite16_BE(u16 i, FILE *f)
{
	u16 p = Low2Big_u16(i);
	fwrite(&p, sizeof(u16), 1, f);
}

void fwrite32_BE(u32 i, FILE *f)
{
	u32 p = Low2Big_u32(i);
	fwrite(&p, sizeof(u32), 1, f);
}

void fwrite64_BE(u64 i, FILE *f)
{
	u64 p = Low2Big_u64(i);
	fwrite(&p, sizeof(u64), 1, f);
}

int fcopy(char *src_name, char *dest_name)
{
	FILE *src=fopen(src_name, "rb");
	if(src == NULL) return -1;
	FILE *dest=fopen(dest_name, "wb");
	fseek(src, 0, 2);
	unsigned int data_size=ftell(src);
	rewind(src);
	unsigned int block_size = 512;
	while(data_size>0)
	{
		char data[512];
		block_size = 512;
		if(data_size<block_size)
			block_size = data_size;
		data_size -= block_size;
		fread(data, 1, block_size, src);
		fwrite(data, 1, block_size, dest);
	}
	fclose(src);
	fclose(dest);
	return 0;
}
