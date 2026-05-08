#include "pch.h"
#include "NES/Mappers/Unlicensed/Coolgirl.h"
#include "NES/NesConsole.h"
#include "NES/NesCpu.h"
#include "NES/NesMemoryManager.h"
#include "NES/BaseNesPpu.h"

static const uint8_t cfi_data[] = {
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x51, 0x52, 0x59, 0x02, 0x00, 0x40, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x27, 0x36, 0x00, 0x00, 0x06,
	0x06, 0x09, 0x13, 0x03, 0x05, 0x03, 0x02, 0x1E,
	0x02, 0x00, 0x06, 0x00, 0x01, 0xFF, 0x03, 0x00,
	0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF,
	0x50, 0x52, 0x49, 0x31, 0x33, 0x14, 0x02, 0x01,
	0x00, 0x08, 0x00, 0x00, 0x02, 0xB5, 0xC5, 0x05,
	0x01, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF
};

Coolgirl::Coolgirl()
{
}

uint8_t Coolgirl::StringToBits(const char* bitsStr, int* bits)
{
	uint8_t bit1, bit2, count = 0;
	for (int i = 0; i < 32; i++)
		bits[i] = -1;
	while (*bitsStr) {
		bit1 = 0;
		bit2 = 0;
		if (isdigit(*bitsStr)) {
			while (isdigit(*bitsStr)) {
				bit1 *= 10;
				bit1 += *bitsStr - '0';
				bitsStr++;
			}
			if (*bitsStr == ':') {
				bitsStr++;
				while (isdigit(*bitsStr)) {
					bit2 *= 10;
					bit2 += *bitsStr - '0';
					bitsStr++;
				}
				if (bit2 < bit1)
					for (int i = bit1; i >= (int)bit2; i--) { bits[count] = i; count++; }
				else
					for (int i = bit1; i <= (int)bit2; i++) { bits[count] = i; count++; }
			} else {
				bits[count] = bit1;
				count++;
			}
		} else {
			bitsStr++;
		}
	}
	return count;
}

uint32_t Coolgirl::GetBits(uint32_t value, const char* bitsStr)
{
	uint32_t result = 0;
	int bits[32];
	StringToBits(bitsStr, bits);
	for (int i = 0; bits[i] >= 0; i++) {
		result <<= 1;
		result |= (value >> bits[i]) & 1;
	}
	return result;
}

uint32_t Coolgirl::SetBits(uint32_t value, const char* bitsStr, uint32_t newValue)
{
	int bits[32];
	uint8_t count = StringToBits(bitsStr, bits);
	for (int i = 0; i < count; i++) {
		if ((newValue >> (count - i - 1)) & 1)
			value |= 1u << bits[i];
		else
			value &= ~(1u << bits[i]);
	}
	return value;
}

void Coolgirl::SyncPrg()
{
	_prgBank6000Mapped = (_prgBase >> 13) | (_prgBank6000 & ((~(_prgMask >> 13) & 0xFE) | 1));
	_prgBankAMapped = (_prgBase >> 13) | (_prgBankA & ((~(_prgMask >> 13) & 0xFE) | 1));
	_prgBankBMapped = (_prgBase >> 13) | (_prgBankB & ((~(_prgMask >> 13) & 0xFE) | 1));
	_prgBankCMapped = (_prgBase >> 13) | (_prgBankC & ((~(_prgMask >> 13) & 0xFE) | 1));
	_prgBankDMapped = (_prgBase >> 13) | (_prgBankD & ((~(_prgMask >> 13) & 0xFE) | 1));

	int regAChip = (_saveFlash.size() > 0 && _prgBankAMapped >= 0x20000 - SAVE_FLASH_SIZE / 1024 / 8) ? FLASH_CHIP : ROM_CHIP;
	int regBChip = (_saveFlash.size() > 0 && _prgBankBMapped >= 0x20000 - SAVE_FLASH_SIZE / 1024 / 8) ? FLASH_CHIP : ROM_CHIP;
	int regCChip = (_saveFlash.size() > 0 && _prgBankCMapped >= 0x20000 - SAVE_FLASH_SIZE / 1024 / 8) ? FLASH_CHIP : ROM_CHIP;
	int regDChip = (_saveFlash.size() > 0 && _prgBankDMapped >= 0x20000 - SAVE_FLASH_SIZE / 1024 / 8) ? FLASH_CHIP : ROM_CHIP;

	if (!_cfiMode || _saveFlash.size() == 0) {
		switch (_prgMode & 7) {
		default:
		case 0:
			if (regAChip == FLASH_CHIP && _saveFlash.size() > 0)
				SetCpuMemoryMapping(0x8000, 0xBFFF, _saveFlash.data(), _prgBankAMapped * 0x2000, _saveFlash.size(), MemoryAccessType::Read);
			else
				SetCpuMemoryMapping(0x8000, 0xBFFF, PrgMemoryType::PrgRom, _prgBankAMapped * 0x2000, MemoryAccessType::Read);
			if (regCChip == FLASH_CHIP && _saveFlash.size() > 0)
				SetCpuMemoryMapping(0xC000, 0xFFFF, _saveFlash.data(), _prgBankCMapped * 0x2000, _saveFlash.size(), MemoryAccessType::Read);
			else
				SetCpuMemoryMapping(0xC000, 0xFFFF, PrgMemoryType::PrgRom, _prgBankCMapped * 0x2000, MemoryAccessType::Read);
			break;
		case 1:
			if (regCChip == FLASH_CHIP && _saveFlash.size() > 0)
				SetCpuMemoryMapping(0x8000, 0xBFFF, _saveFlash.data(), _prgBankCMapped * 0x2000, _saveFlash.size(), MemoryAccessType::Read);
			else
				SetCpuMemoryMapping(0x8000, 0xBFFF, PrgMemoryType::PrgRom, _prgBankCMapped * 0x2000, MemoryAccessType::Read);
			if (regAChip == FLASH_CHIP && _saveFlash.size() > 0)
				SetCpuMemoryMapping(0xC000, 0xFFFF, _saveFlash.data(), _prgBankAMapped * 0x2000, _saveFlash.size(), MemoryAccessType::Read);
			else
				SetCpuMemoryMapping(0xC000, 0xFFFF, PrgMemoryType::PrgRom, _prgBankAMapped * 0x2000, MemoryAccessType::Read);
			break;
		case 4:
			if (regAChip == FLASH_CHIP && _saveFlash.size() > 0)
				SetCpuMemoryMapping(0x8000, 0x9FFF, _saveFlash.data(), _prgBankAMapped * 0x2000, _saveFlash.size(), MemoryAccessType::Read);
			else
				SetCpuMemoryMapping(0x8000, 0x9FFF, PrgMemoryType::PrgRom, _prgBankAMapped * 0x2000, MemoryAccessType::Read);
			if (regBChip == FLASH_CHIP && _saveFlash.size() > 0)
				SetCpuMemoryMapping(0xA000, 0xBFFF, _saveFlash.data(), _prgBankBMapped * 0x2000, _saveFlash.size(), MemoryAccessType::Read);
			else
				SetCpuMemoryMapping(0xA000, 0xBFFF, PrgMemoryType::PrgRom, _prgBankBMapped * 0x2000, MemoryAccessType::Read);
			if (regCChip == FLASH_CHIP && _saveFlash.size() > 0)
				SetCpuMemoryMapping(0xC000, 0xDFFF, _saveFlash.data(), _prgBankCMapped * 0x2000, _saveFlash.size(), MemoryAccessType::Read);
			else
				SetCpuMemoryMapping(0xC000, 0xDFFF, PrgMemoryType::PrgRom, _prgBankCMapped * 0x2000, MemoryAccessType::Read);
			if (regDChip == FLASH_CHIP && _saveFlash.size() > 0)
				SetCpuMemoryMapping(0xE000, 0xFFFF, _saveFlash.data(), _prgBankDMapped * 0x2000, _saveFlash.size(), MemoryAccessType::Read);
			else
				SetCpuMemoryMapping(0xE000, 0xFFFF, PrgMemoryType::PrgRom, _prgBankDMapped * 0x2000, MemoryAccessType::Read);
			break;
		case 5:
			if (regCChip == FLASH_CHIP && _saveFlash.size() > 0)
				SetCpuMemoryMapping(0x8000, 0x9FFF, _saveFlash.data(), _prgBankCMapped * 0x2000, _saveFlash.size(), MemoryAccessType::Read);
			else
				SetCpuMemoryMapping(0x8000, 0x9FFF, PrgMemoryType::PrgRom, _prgBankCMapped * 0x2000, MemoryAccessType::Read);
			if (regBChip == FLASH_CHIP && _saveFlash.size() > 0)
				SetCpuMemoryMapping(0xA000, 0xBFFF, _saveFlash.data(), _prgBankBMapped * 0x2000, _saveFlash.size(), MemoryAccessType::Read);
			else
				SetCpuMemoryMapping(0xA000, 0xBFFF, PrgMemoryType::PrgRom, _prgBankBMapped * 0x2000, MemoryAccessType::Read);
			if (regAChip == FLASH_CHIP && _saveFlash.size() > 0)
				SetCpuMemoryMapping(0xC000, 0xDFFF, _saveFlash.data(), _prgBankAMapped * 0x2000, _saveFlash.size(), MemoryAccessType::Read);
			else
				SetCpuMemoryMapping(0xC000, 0xDFFF, PrgMemoryType::PrgRom, _prgBankAMapped * 0x2000, MemoryAccessType::Read);
			if (regDChip == FLASH_CHIP && _saveFlash.size() > 0)
				SetCpuMemoryMapping(0xE000, 0xFFFF, _saveFlash.data(), _prgBankDMapped * 0x2000, _saveFlash.size(), MemoryAccessType::Read);
			else
				SetCpuMemoryMapping(0xE000, 0xFFFF, PrgMemoryType::PrgRom, _prgBankDMapped * 0x2000, MemoryAccessType::Read);
			break;
		case 6:
			if (regBChip == FLASH_CHIP && _saveFlash.size() > 0)
				SetCpuMemoryMapping(0x8000, 0xFFFF, _saveFlash.data(), (_prgBankBMapped >> 2) * 0x8000, _saveFlash.size(), MemoryAccessType::Read);
			else
				SetCpuMemoryMapping(0x8000, 0xFFFF, PrgMemoryType::PrgRom, (_prgBankBMapped >> 2) * 0x8000, MemoryAccessType::Read);
			break;
		case 7:
			if (regAChip == FLASH_CHIP && _saveFlash.size() > 0)
				SetCpuMemoryMapping(0x8000, 0xFFFF, _saveFlash.data(), (_prgBankAMapped >> 2) * 0x8000, _saveFlash.size(), MemoryAccessType::Read);
			else
				SetCpuMemoryMapping(0x8000, 0xFFFF, PrgMemoryType::PrgRom, (_prgBankAMapped >> 2) * 0x8000, MemoryAccessType::Read);
			break;
		}
	} else {
		SetCpuMemoryMapping(0x8000, 0xFFFF, _cfiData.data(), 0, _cfiData.size(), MemoryAccessType::Read);
	}

	if (!_mapRomOn6000 && _wram.size() > 0 && _sramEnabled) {
		SetCpuMemoryMapping(0x6000, 0x7FFF, _wram.data(), (_sramPage & 0x03) * 0x2000, _wram.size(), MemoryAccessType::ReadWrite);
	} else if (_mapRomOn6000) {
		SetCpuMemoryMapping(0x6000, 0x7FFF, PrgMemoryType::PrgRom, _prgBank6000Mapped * 0x2000, MemoryAccessType::Read);
	} else {
		RemoveCpuMemoryMapping(0x6000, 0x7FFF);
	}
}

void Coolgirl::SyncChr()
{
	int chrShiftRight = ((_mapper == CgMapper::VRC2_VRC4) && (_flags & 0b010)) ? 1 : 0;
	int chrShiftLeft = 0;

	switch (_chrMode & 7) {
	default:
	case 0: SelectChrPage8x(0, _chrBankA >> 3 << 3 >> chrShiftRight << chrShiftLeft, ChrMemoryType::ChrRam); break;
	case 1:
		SelectChrPage4x(0, _mapper163Latch >> chrShiftRight << chrShiftLeft, ChrMemoryType::ChrRam);
		SelectChrPage4x(1, _mapper163Latch >> chrShiftRight << chrShiftLeft, ChrMemoryType::ChrRam);
		break;
	case 2:
		SelectChrPage2x(0, _chrBankA >> 1 << 1 >> chrShiftRight << chrShiftLeft, ChrMemoryType::ChrRam);
		_tksMir[0] = _tksMir[1] = (uint8_t)_chrBankA;
		SelectChrPage2x(1, _chrBankC >> 1 << 1 >> chrShiftRight << chrShiftLeft, ChrMemoryType::ChrRam);
		_tksMir[2] = _tksMir[3] = (uint8_t)_chrBankC;
		SelectChrPage(4, _chrBankE >> chrShiftRight << chrShiftLeft, ChrMemoryType::ChrRam);
		_tksMir[4] = (uint8_t)_chrBankE;
		SelectChrPage(5, _chrBankF >> chrShiftRight << chrShiftLeft, ChrMemoryType::ChrRam);
		_tksMir[5] = (uint8_t)_chrBankF;
		SelectChrPage(6, _chrBankG >> chrShiftRight << chrShiftLeft, ChrMemoryType::ChrRam);
		_tksMir[6] = (uint8_t)_chrBankG;
		SelectChrPage(7, _chrBankH >> chrShiftRight << chrShiftLeft, ChrMemoryType::ChrRam);
		_tksMir[7] = (uint8_t)_chrBankH;
		break;
	case 3:
		SelectChrPage(0, _chrBankE >> chrShiftRight << chrShiftLeft, ChrMemoryType::ChrRam);
		_tksMir[0] = (uint8_t)_chrBankE;
		SelectChrPage(1, _chrBankF >> chrShiftRight << chrShiftLeft, ChrMemoryType::ChrRam);
		_tksMir[1] = (uint8_t)_chrBankF;
		SelectChrPage(2, _chrBankG >> chrShiftRight << chrShiftLeft, ChrMemoryType::ChrRam);
		_tksMir[2] = (uint8_t)_chrBankG;
		SelectChrPage(3, _chrBankH >> chrShiftRight << chrShiftLeft, ChrMemoryType::ChrRam);
		_tksMir[3] = (uint8_t)_chrBankH;
		SelectChrPage2x(2, _chrBankA >> 1 << 1 >> chrShiftRight << chrShiftLeft, ChrMemoryType::ChrRam);
		_tksMir[4] = _tksMir[5] = (uint8_t)_chrBankA;
		SelectChrPage2x(3, _chrBankC >> 1 << 1 >> chrShiftRight << chrShiftLeft, ChrMemoryType::ChrRam);
		_tksMir[6] = _tksMir[7] = (uint8_t)_chrBankC;
		break;
	case 4:
		SelectChrPage4x(0, _chrBankA >> 2 << 2 >> chrShiftRight << chrShiftLeft, ChrMemoryType::ChrRam);
		SelectChrPage4x(1, _chrBankE >> 2 << 2 >> chrShiftRight << chrShiftLeft, ChrMemoryType::ChrRam);
		break;
	case 5:
		if (!_ppuLatch0) SelectChrPage4x(0, _chrBankA >> 2 << 2 >> chrShiftRight << chrShiftLeft, ChrMemoryType::ChrRam);
		else SelectChrPage4x(0, _chrBankB >> 2 << 2 >> chrShiftRight << chrShiftLeft, ChrMemoryType::ChrRam);
		if (!_ppuLatch1) SelectChrPage4x(1, _chrBankE >> 2 << 2 >> chrShiftRight << chrShiftLeft, ChrMemoryType::ChrRam);
		else SelectChrPage4x(1, _chrBankF >> 2 << 2 >> chrShiftRight << chrShiftLeft, ChrMemoryType::ChrRam);
		break;
	case 6:
		SelectChrPage2x(0, _chrBankA >> 1 << 1 >> chrShiftRight << chrShiftLeft, ChrMemoryType::ChrRam);
		SelectChrPage2x(1, _chrBankC >> 1 << 1 >> chrShiftRight << chrShiftLeft, ChrMemoryType::ChrRam);
		SelectChrPage2x(2, _chrBankE >> 1 << 1 >> chrShiftRight << chrShiftLeft, ChrMemoryType::ChrRam);
		SelectChrPage2x(3, _chrBankG >> 1 << 1 >> chrShiftRight << chrShiftLeft, ChrMemoryType::ChrRam);
		break;
	case 7:
		SelectChrPage(0, _chrBankA >> chrShiftRight << chrShiftLeft, ChrMemoryType::ChrRam);
		SelectChrPage(1, _chrBankB >> chrShiftRight << chrShiftLeft, ChrMemoryType::ChrRam);
		SelectChrPage(2, _chrBankC >> chrShiftRight << chrShiftLeft, ChrMemoryType::ChrRam);
		SelectChrPage(3, _chrBankD >> chrShiftRight << chrShiftLeft, ChrMemoryType::ChrRam);
		SelectChrPage(4, _chrBankE >> chrShiftRight << chrShiftLeft, ChrMemoryType::ChrRam);
		SelectChrPage(5, _chrBankF >> chrShiftRight << chrShiftLeft, ChrMemoryType::ChrRam);
		SelectChrPage(6, _chrBankG >> chrShiftRight << chrShiftLeft, ChrMemoryType::ChrRam);
		SelectChrPage(7, _chrBankH >> chrShiftRight << chrShiftLeft, ChrMemoryType::ChrRam);
		break;
	}
}

void Coolgirl::SyncMirroring()
{
	if (!_fourScreen) {
		if (!((_mapper == CgMapper::MMC3_MMC6) && (_flags & 1))) {
			switch (_mirroring) {
			case 0: SetMirroringType(MirroringType::Vertical); break;
			case 1: SetMirroringType(MirroringType::Horizontal); break;
			case 2: SetMirroringType(MirroringType::ScreenAOnly); break;
			case 3: SetMirroringType(MirroringType::ScreenBOnly); break;
			}
		}
	} else {
		SetMirroringType(MirroringType::FourScreens);
	}
}

void Coolgirl::Sync() { SyncPrg(); SyncChr(); SyncMirroring(); }

void Coolgirl::FlashWrite(uint16_t addr, uint8_t value)
{
	if (_flashState < sizeof(_flashBufferA) / sizeof(_flashBufferA[0])) {
		_flashBufferA[_flashState] = addr & 0xFFF;
		_flashBufferV[_flashState] = value;
		_flashState++;

		if ((_flashState == 1) && (_flashBufferA[0] == 0x0AAA) && (_flashBufferV[0] == 0x98)) {
			_cfiMode = 1;
			_flashState = 0;
		}
		if ((_flashState == 6) &&
			(_flashBufferA[0] == 0x0AAA) && (_flashBufferV[0] == 0xAA) &&
			(_flashBufferA[1] == 0x0555) && (_flashBufferV[1] == 0x55) &&
			(_flashBufferA[2] == 0x0AAA) && (_flashBufferV[2] == 0x80) &&
			(_flashBufferA[3] == 0x0AAA) && (_flashBufferV[3] == 0xAA) &&
			(_flashBufferA[4] == 0x0555) && (_flashBufferV[4] == 0x55) &&
			(_flashBufferV[5] == 0x30)) {
			int sector = _prgBankAMapped * 0x2000 / FLASH_SECTOR_SIZE;
			uint32_t sectorAddress = sector * FLASH_SECTOR_SIZE;
			for (uint32_t i = sectorAddress; i < sectorAddress + FLASH_SECTOR_SIZE && i < _saveFlash.size(); i++)
				_saveFlash[i % SAVE_FLASH_SIZE] = 0xFF;
			_flashState = 0;
		}
		if ((_flashState == 4) &&
			(_flashBufferA[0] == 0x0AAA) && (_flashBufferV[0] == 0xAA) &&
			(_flashBufferA[1] == 0x0555) && (_flashBufferV[1] == 0x55) &&
			(_flashBufferA[2] == 0x0AAA) && (_flashBufferV[2] == 0xA0)) {
			uint32_t flashAddr = _prgBankAMapped * 0x2000 + (addr % 0x8000);
			if (_saveFlash.size() > 0 && flashAddr < _saveFlash.size()) {
				if (_saveFlash[flashAddr % SAVE_FLASH_SIZE] == 0xFF)
					_saveFlash[flashAddr % SAVE_FLASH_SIZE] = value;
			}
			_flashState = 0;
		}
	}
	if (((addr & 0xFFF) != 0x0AAA) && ((addr & 0xFFF) != 0x0555))
		_flashState = 0;
	if (value == 0xF0) {
		_flashState = 0;
		_cfiMode = 0;
	}
	SyncPrg();
}

void Coolgirl::HandleSubMapperLogic(uint16_t addr, uint8_t value)
{
	uint16_t A = addr;
	uint8_t V = value;

	if (_mapper == CgMapper::Passthrough) return;

	if (_mapper == CgMapper::UxROM) {
		if (!(_flags & 1) || (GetBits(A, "14:12") != 0b001)) {
			SET_BITS(_prgBankA, "5:1", V, "4:0");
			if (_flags & 2) {
				SET_BITS(_mirroring, "1", 1, "0");
				SET_BITS(_mirroring, "0", V, "7");
				SET_BITS(_chrBankA, "1:0", V, "6:5");
			}
		} else {
			SET_BITS(_mirroring, "1", 1, "0");
			SET_BITS(_mirroring, "0", V, "4");
		}
	}

	if (_mapper == CgMapper::CNROM) {
		SET_BITS(_chrBankA, "7:3", V, "4:0");
	}

	if (_mapper == CgMapper::HolyDiver) {
		SET_BITS(_prgBankA, "3:1", V, "2:0");
		SET_BITS(_chrBankA, "6:3", V, "7:4");
		_mirroring = GetBits(V, "3") ^ 1;
	}

	if (_mapper == CgMapper::TAM_S1) {
		SET_BITS(_prgBankA, "5:1", V, "4:0");
		_mirroring = GetBits(V, "7") ^ 1;
	}

	if (_mapper == CgMapper::Sunsoft2) {
		SET_BITS(_prgBankA, "3:1", V, "6:4");
		_canWriteChr = V & 1;
	}

	if (_mapper == CgMapper::Mapper163) {
		switch (GetBits(A, "14:8") & 0x77) {
		case 0x11:
			_mapper163R0 = V; break;
		case 0x31:
			_mapper163R1 = V; break;
		case 0x51:
			_mapper163R2 = V; break;
		case 0x71:
			_mapper163R3 = V; break;
		case 0x12:
			_mapper163R4 = V; break;
		case 0x32:
			_mapper163R5 = V; break;
		case 0x41:
			_mapper163Latch = 0; break;
		case 0x42:
			_mapper163Latch = 1; break;
		case 0x43:
			SET_BITS(_prgBankA, "3:0", V, "3:0"); break;
		case 0x44:
			SET_BITS(_chrBankA, "6:0", V, "6:0"); break;
		case 0x45:
			SET_BITS(_mirroring, "0", V, "0"); break;
		case 0x46:
			_sramEnabled = V & 1; break;
		}
	}

	if (_mapper == CgMapper::SS88006) {
		switch (GetBits(A, "14:12,1:0")) {
		case 0b00000: SET_BITS(_prgBankA, "3:0", V, "3:0"); break;
		case 0b00001: SET_BITS(_prgBankA, "7:4", V, "3:0"); break;
		case 0b00010: SET_BITS(_prgBankB, "3:0", V, "3:0"); break;
		case 0b00011: SET_BITS(_prgBankB, "7:4", V, "3:0"); break;
		case 0b00100: SET_BITS(_prgBankC, "3:0", V, "3:0"); break;
		case 0b00101: SET_BITS(_prgBankC, "7:4", V, "3:0"); break;
		case 0b00110: break;
		case 0b00111: break;
		case 0b01000: SET_BITS(_chrBankA, "3:0", V, "3:0"); break;
		case 0b01001: SET_BITS(_chrBankA, "7:4", V, "3:0"); break;
		case 0b01010: SET_BITS(_chrBankB, "3:0", V, "3:0"); break;
		case 0b01011: SET_BITS(_chrBankB, "7:4", V, "3:0"); break;
		case 0b01100: SET_BITS(_chrBankC, "3:0", V, "3:0"); break;
		case 0b01101: SET_BITS(_chrBankC, "7:4", V, "3:0"); break;
		case 0b01110: SET_BITS(_chrBankD, "3:0", V, "3:0"); break;
		case 0b01111: SET_BITS(_chrBankD, "7:4", V, "3:0"); break;
		case 0b10000: SET_BITS(_chrBankE, "3:0", V, "3:0"); break;
		case 0b10001: SET_BITS(_chrBankE, "7:4", V, "3:0"); break;
		case 0b10010: SET_BITS(_chrBankF, "3:0", V, "3:0"); break;
		case 0b10011: SET_BITS(_chrBankF, "7:4", V, "3:0"); break;
		case 0b10100: SET_BITS(_chrBankG, "3:0", V, "3:0"); break;
		case 0b10101: SET_BITS(_chrBankG, "7:4", V, "3:0"); break;
		case 0b10110: SET_BITS(_chrBankH, "3:0", V, "3:0"); break;
		case 0b10111: SET_BITS(_chrBankH, "7:4", V, "3:0"); break;
		case 0b11000: SET_BITS(_mapper18IrqLatch, "3:0", V, "3:0"); break;
		case 0b11001: SET_BITS(_mapper18IrqLatch, "7:4", V, "3:0"); break;
		case 0b11010: SET_BITS(_mapper18IrqLatch, "11:8", V, "3:0"); break;
		case 0b11011: SET_BITS(_mapper18IrqLatch, "15:12", V, "3:0"); break;
		case 0b11100:
			_console->GetCpu()->ClearIrqSource(IRQSource::External);
			_mapper18IrqValue = _mapper18IrqLatch; break;
		case 0b11101:
			_console->GetCpu()->ClearIrqSource(IRQSource::External);
			SET_BITS(_mapper18IrqControl, "3:0", V, "3:0"); break;
		case 0b11110:
			switch (GetBits(V, "1:0")) {
			case 0b00: _mirroring = 0b01; break;
			case 0b01: _mirroring = 0b00; break;
			case 0b10: _mirroring = 0b10; break;
			case 0b11: _mirroring = 0b11; break;
			}
			break;
		case 0b11111: break;
		}
	}

	if (_mapper == CgMapper::AxROM) {
		SET_BITS(_prgBankA, "5:2", V, "3:0");
		if (!(_flags & 1))
			_mirroring = (1 << 1) | GetBits(V, "4");
	}

	if (_mapper == CgMapper::Cheetahmen2) {
		SET_BITS(_prgBankA, "5:2", A, "10:7");
		SET_BITS(_chrBankA, "7:5", A, "2:0");
		SET_BITS(_chrBankA, "4:3", V, "1:0");
		_mirroring = GetBits(A, "13");
	}

	if (_mapper == CgMapper::ColorDreams) {
		SET_BITS(_prgBankA, "3:2", V, "1:0");
		SET_BITS(_chrBankA, "6:3", V, "7:4");
	}

	if (_mapper == CgMapper::GxROM) {
		SET_BITS(_prgBankA, "3:2", V, "5:4");
		SET_BITS(_chrBankA, "4:3", V, "1:0");
	}

	if (_mapper == CgMapper::Mapper87) {
		if (GetBits(A, "14:13") == 0b11) {
			SET_BITS(_chrBankA, "4:3", V, "0,1");
		}
	}

	if (_mapper == CgMapper::JY) {
		if (GetBits(A, "14:12") == 0b000) {
			switch (GetBits(A, "1:0")) {
			case 0b00: SET_BITS(_prgBankA, "5:0", V, "5:0"); break;
			case 0b01: SET_BITS(_prgBankB, "5:0", V, "5:0"); break;
			case 0b10: SET_BITS(_prgBankC, "5:0", V, "5:0"); break;
			case 0b11: SET_BITS(_prgBankD, "5:0", V, "5:0"); break;
			}
		}
		if (GetBits(A, "14:12") == 0b001) {
			switch (GetBits(A, "2:0")) {
			case 0b000: SET_BITS(_chrBankA, "7:0", V, "7:0"); break;
			case 0b001: SET_BITS(_chrBankB, "7:0", V, "7:0"); break;
			case 0b010: SET_BITS(_chrBankC, "7:0", V, "7:0"); break;
			case 0b011: SET_BITS(_chrBankD, "7:0", V, "7:0"); break;
			case 0b100: SET_BITS(_chrBankE, "7:0", V, "7:0"); break;
			case 0b101: SET_BITS(_chrBankF, "7:0", V, "7:0"); break;
			case 0b110: SET_BITS(_chrBankG, "7:0", V, "7:0"); break;
			case 0b111: SET_BITS(_chrBankH, "7:0", V, "7:0"); break;
			}
		}
		if (GetBits(A, "14:12,1:0") == 0b10101) {
			SET_BITS(_mirroring, "1:0", V, "1:0");
		}
		if (GetBits(A, "14:12") == 0b100) {
			switch (GetBits(A, "2:0")) {
			case 0b000:
				if (V & 1) _mmc3IrqEnabled = 1;
				else { _console->GetCpu()->ClearIrqSource(IRQSource::External); _mmc3IrqEnabled = 0; }
				break;
			case 0b001: break;
			case 0b010:
				_mmc3IrqEnabled = 0;
				_console->GetCpu()->ClearIrqSource(IRQSource::External);
				break;
			case 0b011: _mmc3IrqEnabled = 1; break;
			case 0b100: break;
			case 0b101:
				_mmc3IrqLatch = V ^ _mapper90Xor;
				_mmc3IrqReload = 1;
				break;
			case 0b110: _mapper90Xor = V; break;
			case 0b111: break;
			}
		}
		if (A == 0x5800) _mul1 = V;
		if (A == 0x5801) _mul2 = V;
	}

	if (_mapper == CgMapper::H3001) {
		switch (GetBits(A, "14:12,2:0")) {
		case 0b000000: SET_BITS(_prgBankA, "5:0", V, "5:0"); break;
		case 0b001001: _mirroring = GetBits(V, "7"); break;
		case 0b001011:
			_console->GetCpu()->ClearIrqSource(IRQSource::External);
			_mapper65IrqEnabled = GetBits(V, "7");
			break;
		case 0b001100:
			_console->GetCpu()->ClearIrqSource(IRQSource::External);
			_mapper65IrqValue = _mapper65IrqLatch;
			break;
		case 0b001101: SET_BITS(_mapper65IrqLatch, "15:8", V, "7:0"); break;
		case 0b001110: SET_BITS(_mapper65IrqLatch, "7:0", V, "7:0"); break;
		case 0b010000: _prgBankB = (_prgBankB & 0b11000000) | (V & 0b00111111); break;
		case 0b011000: SET_BITS(_chrBankA, "7:0", V, "7:0"); break;
		case 0b011001: SET_BITS(_chrBankB, "7:0", V, "7:0"); break;
		case 0b011010: SET_BITS(_chrBankC, "7:0", V, "7:0"); break;
		case 0b011011: SET_BITS(_chrBankD, "7:0", V, "7:0"); break;
		case 0b011100: SET_BITS(_chrBankE, "7:0", V, "7:0"); break;
		case 0b011101: SET_BITS(_chrBankF, "7:0", V, "7:0"); break;
		case 0b011110: SET_BITS(_chrBankG, "7:0", V, "7:0"); break;
		case 0b011111: SET_BITS(_chrBankH, "7:0", V, "7:0"); break;
		case 0b100000: SET_BITS(_prgBankC, "5:0", V, "5:0"); break;
		}
	}

	if (_mapper == CgMapper::MMC5) {
		switch (GetBits(A, "14:13")) {
		case 0b00:
			SET_BITS(_prgBankA, "6:0", V, "6:0"); break;
		case 0b01:
			SET_BITS(_prgBankB, "6:0", V, "6:0"); break;
		case 0b10:
			SET_BITS(_prgBankC, "6:0", V, "6:0"); break;
		case 0b11:
			if (A == 0x5204) {
				_mmc5IrqOut = 0;
				_console->GetCpu()->ClearIrqSource(IRQSource::External);
			}
			break;
		}
		if (A == 0x5105) SET_BITS(_mirroring, "1:0", V, "1:0");
		if (A == 0x5203) _mmc5IrqLine = V;
		if (A == 0x5204) _mmc5IrqEnabled = GetBits(V, "7");
	}

	if (_mapper == CgMapper::MMC1) {
		if (V & 0x80) {
			_mmc1LoadRegister = SetBits(_mmc1LoadRegister, "5:0", 100000);
			_prgMode = 0;
			_prgBankC = SetBits(_prgBankC, "4:0", 11110);
		} else {
			SET_BITS(_mmc1LoadRegister, "4:0", _mmc1LoadRegister, "5:1");
			_mmc1LoadRegister = SetBits(_mmc1LoadRegister, "5", GetBits(V, "0"));
			if (_mmc1LoadRegister & 1) {
				switch ((A >> 13) & 3) {
				case 0b00:
					if (GetBits(_mmc1LoadRegister, "4:3") == 0b11) {
						_prgMode = 0;
						_prgBankC = SetBits(_prgBankC, "4:1", 1111);
					} else if (GetBits(_mmc1LoadRegister, "4:3") == 0b10) {
						_prgMode = 0b001;
						_prgBankC = SetBits(_prgBankC, "4:1", 0000);
					} else {
						_prgMode = 0b111;
					}
					if (GetBits(_mmc1LoadRegister, "5"))
						_chrMode = 0b100;
					else
						_chrMode = 0b000;
					_mirroring = SetBits(_mirroring, "1:0", GetBits(_mmc1LoadRegister, "2:1") ^ 0b10);
					break;
				case 0b01:
					SET_BITS(_chrBankA, "6:2", _mmc1LoadRegister, "5:1");
					if (_flags & 1)
						_sramPage = 2 | (GetBits(_mmc1LoadRegister, "4") ^ 1);
					SET_BITS(_prgBankA, "5", _mmc1LoadRegister, "5");
					SET_BITS(_prgBankC, "5", _mmc1LoadRegister, "5");
					break;
				case 0b10:
					SET_BITS(_chrBankE, "6:2", _mmc1LoadRegister, "5:1");
					break;
				case 0b11:
					SET_BITS(_prgBankA, "4:1", _mmc1LoadRegister, "4:1");
					_sramEnabled = GetBits(_mmc1LoadRegister, "5") ^ 1;
					break;
				}
				_mmc1LoadRegister = 0b100000;
			}
		}
	}

	if (_mapper == CgMapper::MMC2_MMC4) {
		switch ((A >> 12) & 7) {
		case 2:
			if (!(_flags & 1)) SET_BITS(_prgBankA, "3:0", V, "3:0");
			else { SET_BITS(_prgBankA, "4:1", V, "3:0"); _prgBankA &= ~1; }
			break;
		case 3: SET_BITS(_chrBankA, "6:2", V, "4:0"); break;
		case 4: SET_BITS(_chrBankB, "6:2", V, "4:0"); break;
		case 5: SET_BITS(_chrBankE, "6:2", V, "4:0"); break;
		case 6: SET_BITS(_chrBankF, "6:2", V, "4:0"); break;
		case 7: _mirroring = V & 1; break;
		}
	}

	if (_mapper == CgMapper::Bandai152) {
		SET_BITS(_chrBankA, "6:3", V, "3:0");
		SET_BITS(_prgBankA, "3:1", V, "6:4");
		if (_flags & 1)
			_mirroring = 2 | GetBits(V, "7");
		else
			SET_BITS(_prgBankA, "4", V, "7");
	}

	if (_mapper == CgMapper::VRC3) {
		switch (GetBits(A, "14:12")) {
		case 0b000: SET_BITS(_vrc3IrqLatch, "3:0", V, "3:0"); break;
		case 0b001: SET_BITS(_vrc3IrqLatch, "7:4", V, "3:0"); break;
		case 0b010: SET_BITS(_vrc3IrqLatch, "11:8", V, "3:0"); break;
		case 0b011: SET_BITS(_vrc3IrqLatch, "15:12", V, "3:0"); break;
		case 0b100:
			_console->GetCpu()->ClearIrqSource(IRQSource::External);
			SET_BITS(_vrc3IrqControl, "2:0", V, "2:0");
			if (_vrc3IrqControl & 2)
				_vrc3IrqValue = _vrc3IrqLatch;
			break;
		case 0b101:
			_console->GetCpu()->ClearIrqSource(IRQSource::External);
			SET_BITS(_vrc3IrqControl, "1", _vrc3IrqControl, "0");
			break;
		case 0b110: break;
		case 0b111: SET_BITS(_prgBankA, "3:1", V, "2:0"); break;
		}
	}

	if (_mapper == CgMapper::MMC3_MMC6) {
		switch (GetBits(A, "14:13,0")) {
		case 0b000:
			SET_BITS(_mmc3Internal, "2:0", V, "2:0");
			if (!(_flags & 2) && !(_flags & 4)) {
				_prgMode = GetBits(V, "6") ? 0b101 : 0b100;
			}
			if (!(_flags & 4)) {
				_chrMode = (V & 0x80) ? 0b011 : 0b010;
			}
			break;
		case 0b001:
			switch (GetBits(_mmc3Internal, "2:0")) {
			case 0b000: SET_BITS(_chrBankA, "7:0", V, "7:0"); break;
			case 0b001: SET_BITS(_chrBankC, "7:0", V, "7:0"); break;
			case 0b010: SET_BITS(_chrBankE, "7:0", V, "7:0"); break;
			case 0b011: SET_BITS(_chrBankF, "7:0", V, "7:0"); break;
			case 0b100: SET_BITS(_chrBankG, "7:0", V, "7:0"); break;
			case 0b101: SET_BITS(_chrBankH, "7:0", V, "7:0"); break;
			case 0b110: if (!(_flags & 2)) SET_BITS(_prgBankA, "7:0", V, "7:0"); break;
			case 0b111: if (!(_flags & 2)) SET_BITS(_prgBankB, "7:0", V, "7:0"); break;
			}
			break;
		case 0b010:
			if (!(_flags & 4)) _mirroring = V & 1;
			break;
		case 0b011: break;
		case 0b100: _mmc3IrqLatch = V; break;
		case 0b101: _mmc3IrqReload = 1; break;
		case 0b110:
			_console->GetCpu()->ClearIrqSource(IRQSource::External);
			_mmc3IrqEnabled = 0;
			break;
		case 0b111:
			if (!(_flags & 4)) _mmc3IrqEnabled = 1;
			break;
		}
	}

	if (_mapper == CgMapper::Mapper112) {
		switch (GetBits(A, "14:13")) {
		case 0b00: SET_BITS(_mapper112Internal, "2:0", V, "2:0"); break;
		case 0b01:
			switch (GetBits(_mapper112Internal, "2:0")) {
			case 0b000: SET_BITS(_prgBankA, "5:0", V, "5:0"); break;
			case 0b001: SET_BITS(_prgBankB, "5:0", V, "5:0"); break;
			case 0b010: SET_BITS(_chrBankA, "7:0", V, "7:0"); break;
			case 0b011: SET_BITS(_chrBankC, "7:0", V, "7:0"); break;
			case 0b100: SET_BITS(_chrBankE, "7:0", V, "7:0"); break;
			case 0b101: SET_BITS(_chrBankF, "7:0", V, "7:0"); break;
			case 0b110: SET_BITS(_chrBankG, "7:0", V, "7:0"); break;
			case 0b111: SET_BITS(_chrBankH, "7:0", V, "7:0"); break;
			}
			break;
		case 0b10: break;
		case 0b11: _mirroring = V & 1; break;
		}
	}

	if (_mapper == CgMapper::Taito) {
		switch (GetBits(A, "14:13,1:0")) {
		case 0b0000:
			SET_BITS(_prgBankA, "5:0", V, "5:0");
			if (!(_flags & 1)) _mirroring = GetBits(V, "6");
			break;
		case 0b0001: SET_BITS(_prgBankB, "5:0", V, "5:0"); break;
		case 0b0010: SET_BITS(_chrBankA, "7:1", V, "6:0"); break;
		case 0b0011: SET_BITS(_chrBankC, "7:1", V, "6:0"); break;
		case 0b0100: SET_BITS(_chrBankE, "7:0", V, "7:0"); break;
		case 0b0101: SET_BITS(_chrBankF, "7:0", V, "7:0"); break;
		case 0b0110: SET_BITS(_chrBankG, "7:0", V, "7:0"); break;
		case 0b0111: SET_BITS(_chrBankH, "7:0", V, "7:0"); break;
		case 0b1100: if (_flags & 1) _mirroring = GetBits(V, "6"); break;
		case 0b1000: _mmc3IrqLatch = SetBits(_mmc3IrqLatch, "7:0", GetBits(V, "7:0") ^ 0xFF); break;
		case 0b1001: _mmc3IrqReload = 1; break;
		case 0b1010: _mmc3IrqEnabled = 1; break;
		case 0b1011:
			_mmc3IrqEnabled = 0;
			_console->GetCpu()->ClearIrqSource(IRQSource::External);
			break;
		}
	}

	if (_mapper == CgMapper::Mapper42) {
		switch (GetBits(A, "14,1:0")) {
		case 0: SET_BITS(_chrBankA, "7:3", V, "4:0"); break;
		case 4: SET_BITS(_prgBank6000, "3:0", V, "3:0"); break;
		case 5: _mirroring = GetBits(V, "3"); break;
		case 6:
			_mapper42IrqEnabled = GetBits(V, "1");
			if (!_mapper42IrqEnabled) {
				_console->GetCpu()->ClearIrqSource(IRQSource::External);
				_mapper42IrqValue = 0;
			}
			break;
		}
	}

	if (_mapper == CgMapper::VRC2_VRC4) {
		uint8_t vrc2bHi =
			(_flags & 5) == 0 ? (GetBits(A, "7") | GetBits(A, "2")) :
			(_flags & 5) == 1 ? GetBits(A, "0") :
			(_flags & 5) == 4 ? (GetBits(A, "5") | GetBits(A, "3") | GetBits(A, "1")) :
			(GetBits(A, "2") | GetBits(A, "0"));
		uint8_t vrc2bLow =
			(_flags & 5) == 0 ? (GetBits(A, "6") | GetBits(A, "1")) :
			(_flags & 5) == 1 ? GetBits(A, "1") :
			(_flags & 5) == 4 ? (GetBits(A, "4") | GetBits(A, "2") | GetBits(A, "0")) :
			(GetBits(A, "3") | GetBits(A, "1"));

		switch ((GetBits(A, "14:12") << 2) | (vrc2bHi << 1) | vrc2bLow) {
		case 0b00000: case 0b00001: case 0b00010: case 0b00011:
			SET_BITS(_prgBankA, "4:0", V, "4:0"); break;
		case 0b00100: case 0b00101:
			if (V != 0xFF) SET_BITS(_mirroring, "1:0", V, "1:0");
			break;
		case 0b00110: case 0b00111:
			SET_BITS(_prgMode, "0", V, "1"); break;
		case 0b01000: case 0b01001: case 0b01010: case 0b01011:
			SET_BITS(_prgBankB, "4:0", V, "4:0"); break;
		case 0b01100: SET_BITS(_chrBankA, "3:0", V, "3:0"); break;
		case 0b01101: SET_BITS(_chrBankA, "7:4", V, "3:0"); break;
		case 0b01110: SET_BITS(_chrBankB, "3:0", V, "3:0"); break;
		case 0b01111: SET_BITS(_chrBankB, "7:4", V, "3:0"); break;
		case 0b10000: SET_BITS(_chrBankC, "3:0", V, "3:0"); break;
		case 0b10001: SET_BITS(_chrBankC, "7:4", V, "3:0"); break;
		case 0b10010: SET_BITS(_chrBankD, "3:0", V, "3:0"); break;
		case 0b10011: SET_BITS(_chrBankD, "7:4", V, "3:0"); break;
		case 0b10100: SET_BITS(_chrBankE, "3:0", V, "3:0"); break;
		case 0b10101: SET_BITS(_chrBankE, "7:4", V, "3:0"); break;
		case 0b10110: SET_BITS(_chrBankF, "3:0", V, "3:0"); break;
		case 0b10111: SET_BITS(_chrBankF, "7:4", V, "3:0"); break;
		case 0b11000: SET_BITS(_chrBankG, "3:0", V, "3:0"); break;
		case 0b11001: SET_BITS(_chrBankG, "7:4", V, "3:0"); break;
		case 0b11010: SET_BITS(_chrBankH, "3:0", V, "3:0"); break;
		case 0b11011: SET_BITS(_chrBankH, "7:4", V, "3:0"); break;
		}

		if (GetBits(A, "14:12") == 0b111) {
			switch ((vrc2bHi << 1) | vrc2bLow) {
			case 0b00:
				SET_BITS(_vrc4IrqLatch, "3:0", V, "3:0"); break;
			case 0b01:
				SET_BITS(_vrc4IrqLatch, "7:4", V, "3:0"); break;
			case 0b10:
				_console->GetCpu()->ClearIrqSource(IRQSource::External);
				SET_BITS(_vrc4IrqControl, "2:0", V, "2:0");
				if (_vrc4IrqControl & 2) {
					_vrc4IrqPrescalerCounter = 0;
					_vrc4IrqPrescaler = 0;
					SET_BITS(_vrc4IrqValue, "7:0", _vrc4IrqLatch, "7:0");
				}
				break;
			case 0b11:
				_console->GetCpu()->ClearIrqSource(IRQSource::External);
				SET_BITS(_vrc4IrqControl, "1", _vrc4IrqControl, "0");
				break;
			}
		}
	}

	if (_mapper == CgMapper::FME7) {
		if (GetBits(A, "14:13") == 0b00) SET_BITS(_mapper69Internal, "3:0", V, "3:0");
		if (GetBits(A, "14:13") == 0b01) {
			switch (GetBits(_mapper69Internal, "3:0")) {
			case 0b0000: SET_BITS(_chrBankA, "7:0", V, "7:0"); break;
			case 0b0001: SET_BITS(_chrBankB, "7:0", V, "7:0"); break;
			case 0b0010: SET_BITS(_chrBankC, "7:0", V, "7:0"); break;
			case 0b0011: SET_BITS(_chrBankD, "7:0", V, "7:0"); break;
			case 0b0100: SET_BITS(_chrBankE, "7:0", V, "7:0"); break;
			case 0b0101: SET_BITS(_chrBankF, "7:0", V, "7:0"); break;
			case 0b0110: SET_BITS(_chrBankG, "7:0", V, "7:0"); break;
			case 0b0111: SET_BITS(_chrBankH, "7:0", V, "7:0"); break;
			case 0b1000:
				_sramEnabled = (V >> 7) & 1;
				_mapRomOn6000 = ((V >> 6) & 1) ^ 1;
				_prgBank6000 = V & 0x3F;
				break;
			case 0b1001: SET_BITS(_prgBankA, "5:0", V, "5:0"); break;
			case 0b1010: SET_BITS(_prgBankB, "5:0", V, "5:0"); break;
			case 0b1011: SET_BITS(_prgBankC, "5:0", V, "5:0"); break;
			case 0b1100: SET_BITS(_mirroring, "1:0", V, "1:0"); break;
			case 0b1101:
				_console->GetCpu()->ClearIrqSource(IRQSource::External);
				_mapper69CounterEnabled = GetBits(V, "7");
				_mapper69IrqEnabled = GetBits(V, "0");
				break;
			case 0b1110: SET_BITS(_mapper69IrqValue, "7:0", V, "7:0"); break;
			case 0b1111: SET_BITS(_mapper69IrqValue, "15:8", V, "7:0"); break;
			}
		}
	}

	if (_mapper == CgMapper::IremG101) {
		switch (GetBits(A, "14:12")) {
		case 0b000: SET_BITS(_prgBankA, "5:0", V, "5:0"); break;
		case 0b001:
			SET_BITS(_prgMode, "0", V, "1");
			_mirroring = V & 1;
			break;
		case 0b010: SET_BITS(_prgBankB, "5:0", V, "5:0"); break;
		case 0b011:
			switch (GetBits(A, "2:0")) {
			case 0b000: SET_BITS(_chrBankA, "7:0", V, "7:0"); break;
			case 0b001: SET_BITS(_chrBankB, "7:0", V, "7:0"); break;
			case 0b010: SET_BITS(_chrBankC, "7:0", V, "7:0"); break;
			case 0b011: SET_BITS(_chrBankD, "7:0", V, "7:0"); break;
			case 0b100: SET_BITS(_chrBankE, "7:0", V, "7:0"); break;
			case 0b101: SET_BITS(_chrBankF, "7:0", V, "7:0"); break;
			case 0b110: SET_BITS(_chrBankG, "7:0", V, "7:0"); break;
			case 0b111: SET_BITS(_chrBankH, "7:0", V, "7:0"); break;
			}
			break;
		}
	}

	if (_mapper == CgMapper::Mapper36) {
		if (GetBits(A, "14:1") != 0b11111111111111) {
			SET_BITS(_prgBankA, "5:2", V, "7:4");
			SET_BITS(_chrBankA, "6:3", V, "3:0");
		}
	}

	if (_mapper == CgMapper::Mapper70) {
		SET_BITS(_prgBankA, "4:1", V, "7:4");
		SET_BITS(_chrBankA, "6:3", V, "3:0");
	}

	if (_mapper == CgMapper::Mapper184) {
		if (GetBits(A, "14:13") == 0b11) {
			SET_BITS(_chrBankA, "4:2", V, "2:0");
			SET_BITS(_chrBankE, "3:2", V, "5:4");
			SET_BITS(_chrBankE, "4", 1, "0");
		}
	}

	if (_mapper == CgMapper::Mapper38) {
		if (GetBits(A, "14:12") == 0b111) {
			SET_BITS(_prgBankA, "3:2", V, "1:0");
			SET_BITS(_chrBankA, "4:3", V, "3:2");
		}
	}

	if (_mapper == CgMapper::VRC1) {
		switch (GetBits(A, "14:12")) {
		case 0b000: SET_BITS(_prgBankA, "3:0", V, "3:0"); break;
		case 0b001:
			_mirroring = V & 1;
			SET_BITS(_chrBankA, "6", V, "1");
			SET_BITS(_chrBankE, "6", V, "2");
			break;
		case 0b010: SET_BITS(_prgBankB, "3:0", V, "3:0"); break;
		case 0b100: SET_BITS(_prgBankC, "3:0", V, "3:0"); break;
		case 0b110: SET_BITS(_chrBankA, "5:2", V, "3:0"); break;
		case 0b111: SET_BITS(_chrBankE, "5:2", V, "3:0"); break;
		}
	}

	if (_mapper == CgMapper::ConyYoko) {
		switch (GetBits(A, "9:8")) {
		case 0b00: SET_BITS(_prgBankA, "4:1", V, "3:0"); break;
		case 0b01:
			_mirroring = GetBits(V, "1:0");
			SET_BITS(_prgMode, "2", V, "4");
			_mapRomOn6000 = GetBits(V, "5");
			_mapper83IrqEnabledLatch = GetBits(V, "7");
			break;
		case 0b10:
			if (!GetBits(A, "0")) {
				_console->GetCpu()->ClearIrqSource(IRQSource::External);
				SET_BITS(_mapper83IrqCounter, "7:0", V, "7:0");
			} else {
				_mapper83IrqEnabled = _mapper83IrqEnabledLatch;
				SET_BITS(_mapper83IrqCounter, "15:8", V, "7:0");
			}
			break;
		case 0b11:
			if (!GetBits(A, "4")) {
				switch (GetBits(A, "1:0")) {
				case 0b00: SET_BITS(_prgBankA, "7:0", V, "7:0"); break;
				case 0b01: SET_BITS(_prgBankB, "7:0", V, "7:0"); break;
				case 0b10: SET_BITS(_prgBankB, "7:0", V, "7:0"); break;
				case 0b11: SET_BITS(_prgBank6000, "7:0", V, "7:0"); break;
				}
			} else {
				if (!(_flags & 0b100)) {
					switch (GetBits(A, "2:0")) {
					case 0b000: SET_BITS(_chrBankA, "7:0", V, "7:0"); break;
					case 0b001: SET_BITS(_chrBankB, "7:0", V, "7:0"); break;
					case 0b010: SET_BITS(_chrBankC, "7:0", V, "7:0"); break;
					case 0b011: SET_BITS(_chrBankD, "7:0", V, "7:0"); break;
					case 0b100: SET_BITS(_chrBankE, "7:0", V, "7:0"); break;
					case 0b101: SET_BITS(_chrBankF, "7:0", V, "7:0"); break;
					case 0b110: SET_BITS(_chrBankG, "7:0", V, "7:0"); break;
					case 0b111: SET_BITS(_chrBankH, "7:0", V, "7:0"); break;
					}
				} else {
					switch (GetBits(A, "2:0")) {
					case 0b000: SET_BITS(_chrBankA, "8:1", V, "7:0"); break;
					case 0b001: SET_BITS(_chrBankC, "8:1", V, "7:0"); break;
					case 0b110: SET_BITS(_chrBankE, "8:1", V, "7:0"); break;
					case 0b111: SET_BITS(_chrBankG, "8:1", V, "7:0"); break;
					}
				}
			}
			break;
		}
	}

	if (_mapper == CgMapper::Sunsoft3) {
		if (GetBits(A, "11")) {
			switch (GetBits(A, "14:12")) {
			case 0b000: SET_BITS(_chrBankA, "6:1", V, "5:0"); break;
			case 0b001: SET_BITS(_chrBankC, "6:1", V, "5:0"); break;
			case 0b010: SET_BITS(_chrBankE, "6:1", V, "5:0"); break;
			case 0b011: SET_BITS(_chrBankG, "6:1", V, "5:0"); break;
			case 0b100:
				_mapper67IrqLatch = ~_mapper67IrqLatch;
				if (_mapper67IrqLatch) SET_BITS(_mapper67IrqCounter, "15:8", V, "7:0");
				else SET_BITS(_mapper67IrqCounter, "7:0", V, "7:0");
				break;
			case 0b101:
				_mapper67IrqLatch = 0;
				SET_BITS(_mapper67IrqEnabled, "0", V, "4");
				break;
			case 0b110: SET_BITS(_mirroring, "1:0", V, "1:0"); break;
			case 0b111: SET_BITS(_prgBankA, "4:1", V, "3:0"); break;
			}
		} else {
			_console->GetCpu()->ClearIrqSource(IRQSource::External);
		}
	}

	if (_mapper == CgMapper::Sunsoft2On3) {
		SET_BITS(_prgBankA, "3:1", V, "6:4");
		SET_BITS(_chrBankA, "6:3", V, "7,2:0");
		SET_BITS(_mirroring, "1", 1, "0");
		SET_BITS(_mirroring, "0", V, "3");
	}

	Sync();
}

void Coolgirl::WriteRegister(uint16_t addr, uint8_t value)
{
	if (_sramEnabled && addr >= 0x6000 && addr < 0x8000 && !_mapRomOn6000 && _wram.size() > 0) {
		_wram[(_sramPage & 0x03) * 0x2000 + (addr & 0x1FFF)] = value;
	}
	if (_saveFlash.size() > 0 && _canWriteFlash && addr >= 0x8000) {
		FlashWrite(addr, value);
	}

	if (_writeDelay) return;
	_writeDelay = 1;

	if (addr >= 0x5000 && addr < 0x6000 && !_lockout) {
		switch (addr & 7) {
		case 0:
			SET_BITS(_prgBase, "29:22", value, "7:0");
			break;
		case 1:
			SET_BITS(_prgBase, "21:14", value, "7:0");
			break;
		case 2:
			SET_BITS(_chrMask, "18", value, "7");
			SET_BITS(_prgMask, "20:14", value, "6:0");
			break;
		case 3:
			SET_BITS(_prgMode, "2:0", value, "7:5");
			SET_BITS(_chrBankA, "7:3", value, "4:0");
			break;
		case 4:
			SET_BITS(_chrMode, "2:0", value, "7:5");
			SET_BITS(_chrMask, "17:13", value, "4:0");
			break;
		case 5:
			SET_BITS(_chrBankA, "8", value, "7");
			SET_BITS(_prgBankA, "5:1", value, "6:2");
			SET_BITS(_sramPage, "1:0", value, "1:0");
			break;
		case 6:
			SET_BITS(_flags, "2:0", value, "7:5");
			_mapper = (CgMapper)SetBits((uint32_t)_mapper, "4:0", GetBits(value, "4:0"));
			break;
		case 7:
			SET_BITS(_lockout, "0", value, "7");
			_mapper = (CgMapper)SetBits((uint32_t)_mapper, "5", GetBits(value, "6"));
			SET_BITS(_fourScreen, "0", value, "5");
			SET_BITS(_mirroring, "1:0", value, "4:3");
			SET_BITS(_canWriteFlash, "0", value, "2");
			SET_BITS(_canWriteChr, "0", value, "1");
			SET_BITS(_sramEnabled, "0", value, "0");
			if (_mapper == CgMapper::MMC2_MMC4) _prgBankB = (uint8_t)~2;
			if (_mapper == CgMapper::Mapper42) _mapRomOn6000 = 1;
			if (_mapper == CgMapper::H3001) _prgBankB = 1;
			break;
		}
		Sync();
	}

	if (addr < 0x8000) {
		if (_mapper == CgMapper::Mapper163) {
			if (addr == 0x5101) {
				if (_mapper163R4 && !value) _mapper163R5 ^= 1;
				_mapper163R4 = value;
			} else if (addr == 0x5100 && value == 6) {
				SET_BITS(_prgMode, "0", 0, "0");
				_prgBankB = 0b1100;
			} else {
				if (GetBits(addr, "14:12") == 0b101) {
					switch (GetBits(addr, "9:8")) {
					case 2:
						SET_BITS(_prgMode, "0", 1, "0");
						SET_BITS(_prgBankA, "7:6", value, "1:0");
						_mapper163R0 = value;
						break;
					case 0:
						SET_BITS(_prgMode, "0", 1, "0");
						SET_BITS(_prgBankA, "5:2", value, "3:0");
						SET_BITS(_chrMode, "0", value, "7");
						_mapper163R1 = value;
						break;
					case 3: _mapper163R2 = value; break;
					case 1: _mapper163R3 = value; break;
					}
				}
			}
		}

		if (_mapper == CgMapper::Mapper87) {
			if (GetBits(addr, "14:13") == 0b11) {
				SET_BITS(_chrBankA, "4:3", value, "0,1");
			}
		}

		if (_mapper == CgMapper::MMC5) {
			switch (addr) {
			case 0x5105:
				if (value == 0xFF) {
					_fourScreen = 1;
				} else {
					_fourScreen = 0;
					switch (GetBits(value, "4,2")) {
					case 0b00: _mirroring = 0b10; break;
					case 0b01: _mirroring = 0b00; break;
					case 0b10: _mirroring = 0b01; break;
					case 0b11: _mirroring = 0b11; break;
					}
				}
				break;
			case 0x5113:
				_sramEnabled = 1;
				SET_BITS(_sramPage, "1:0", value, "1:0");
				break;
			case 0x5114:
				SET_BITS(_prgBankA, "4:1", value, "4:1");
				SET_BITS(_prgBankA, "0", 0, "0");
				SET_BITS(_prgBankB, "4:1", value, "4:1");
				SET_BITS(_prgBankB, "0", 1, "0");
				break;
			case 0x5115:
				SET_BITS(_prgBankA, "4:1", value, "4:1");
				SET_BITS(_prgBankA, "0", 0, "0");
				SET_BITS(_prgBankB, "4:1", value, "4:1");
				SET_BITS(_prgBankB, "0", 1, "0");
				break;
			case 0x5116:
				SET_BITS(_prgBankC, "4:0", value, "4:0");
				break;
			case 0x5117:
				SET_BITS(_prgBankD, "4:0", value, "4:0");
				break;
			case 0x5120: SET_BITS(_chrBankA, "7:0", value, "7:0"); break;
			case 0x5121: SET_BITS(_chrBankB, "7:0", value, "7:0"); break;
			case 0x5122: SET_BITS(_chrBankC, "7:0", value, "7:0"); break;
			case 0x5123: SET_BITS(_chrBankD, "7:0", value, "7:0"); break;
			case 0x5128: SET_BITS(_chrBankE, "7:0", value, "7:0"); break;
			case 0x5129: SET_BITS(_chrBankF, "7:0", value, "7:0"); break;
			case 0x512A: SET_BITS(_chrBankG, "7:0", value, "7:0"); break;
			case 0x512B: SET_BITS(_chrBankH, "7:0", value, "7:0"); break;
			case 0x5203:
				_console->GetCpu()->ClearIrqSource(IRQSource::External);
				_mmc5IrqOut = 0;
				SET_BITS(_mmc5IrqLine, "7:0", value, "7:0");
				break;
			case 0x5204:
				_console->GetCpu()->ClearIrqSource(IRQSource::External);
				_mmc5IrqOut = 0;
				_mmc5IrqEnabled = GetBits(value, "7");
				break;
			}
		}

		if ((_mapper == CgMapper::MMC3_MMC6) && (_flags & 2)) {
			if (addr >= 0x4120) {
				_prgBankA = SetBits(_prgBankA, "5:2", GetBits(value, "7:4") | GetBits(value, "3:0"));
			}
		}

		if (_mapper == CgMapper::NINA03_06) {
			if (GetBits(addr, "14:13,8") == 0b101) {
				SET_BITS(_chrBankA, "5:3", value, "2:0");
				SET_BITS(_prgBankA, "2", value, "3");
			}
		}

		if (_mapper == CgMapper::Mapper133) {
			if (GetBits(addr, "14:13,8") == 0b101) {
				SET_BITS(_chrBankA, "4:3", value, "1:0");
				SET_BITS(_prgBankA, "2", value, "2");
			}
		}
	} else {
		HandleSubMapperLogic(addr, value);
	}
}

uint8_t Coolgirl::ReadRegister(uint16_t addr)
{
	if ((_mapper == CgMapper::Passthrough) && (addr >= 0x5000) && (addr < 0x6000))
		return 0;

	if ((_mapper == CgMapper::Mapper163) && ((addr & 0x7700) == 0x5100))
		return _mapper163R2 | _mapper163R0 | _mapper163R1 | ~_mapper163R3;

	if ((_mapper == CgMapper::Mapper163) && ((addr & 0x7700) == 0x5500))
		return (_mapper163R5 & 1) ? _mapper163R2 : _mapper163R1;

	if ((_mapper == CgMapper::MMC5) && (addr == 0x5204)) {
		int32_t scanline = _console->GetPpu()->GetCurrentScanline();
		uint8_t r = (scanline + 1 >= 241) ? 0 : 1;
		uint8_t p = _mmc5IrqOut;
		_console->GetCpu()->ClearIrqSource(IRQSource::External);
		_mmc5IrqOut = 0;
		return (p << 7) | (r << 6);
	}

	if ((_mapper == CgMapper::Mapper36) && ((addr & 0xE100) == 0x4100))
		return (_prgBankA & 0x0C) << 2;

	if ((_mapper == CgMapper::ConyYoko) && ((addr & 0x7000) == 0x5000)) return _flags & 3;

	if ((_mapper == CgMapper::JY) && (addr == 0x5800)) return (_mul1 * _mul2) & 0xFF;
	if ((_mapper == CgMapper::JY) && (addr == 0x5801)) return ((_mul1 * _mul2) >> 8) & 0xFF;

	if (_sramEnabled && !_mapRomOn6000 && (addr >= 0x6000) && (addr < 0x8000) && _wram.size() > 0)
		return _wram[(_sramPage & 0x03) * 0x2000 + (addr & 0x1FFF)];

	if (_mapRomOn6000 && (addr >= 0x6000) && (addr < 0x8000))
		return InternalReadRam(addr);

	if (addr >= 0x8000)
		return InternalReadRam(addr);

	return _console->GetMemoryManager()->GetOpenBus();
}

void Coolgirl::InitMapper()
{
	AddRegisterRange(0x8000, 0xFFFF, MemoryOperation::Write);
	AddRegisterRange(0x8000, 0xFFFF, MemoryOperation::Read);
	AddRegisterRange(0x4020, 0x4FFF, MemoryOperation::Any);
	AddRegisterRange(0x6000, 0x7FFF, MemoryOperation::Any);

	_chrSize = _chrRomSize > 0 ? _chrRomSize : std::max(_chrRamSize, (uint32_t)GetChrRamSize());
	_wramSize = 32 * 1024;
	_wram.resize(_wramSize, 0);

	if (_chrRomSize == 0 && _chrRamSize < GetChrRamSize()) {
		InitializeChrRam(GetChrRamSize());
	}

	if (HasBattery()) {
		_saveFlash.resize(SAVE_FLASH_SIZE, 0xFF);
		_cfiData.resize(sizeof(cfi_data) * 2);
		for (size_t i = 0; i < sizeof(cfi_data); i++) {
			_cfiData[i * 2] = _cfiData[i * 2 + 1] = cfi_data[i];
		}
	}

	_orgPrgRom = vector<uint8_t>(_prgRom, _prgRom + _prgSize);

	Reset(false);
}

void Coolgirl::Reset(bool softReset)
{
	_sramEnabled = 0;
	_sramPage = 0;
	_canWriteChr = 0;
	_mapRomOn6000 = 0;
	_flags = 0;
	_mapper = CgMapper::Passthrough;
	_canWriteFlash = 0;
	_mirroring = 0;
	_fourScreen = 0;
	_lockout = 0;
	_prgBase = 0;
	_prgMask = 0b11111000 << 14;
	_prgMode = 0;
	_prgBank6000 = 0;
	_prgBankA = 0;
	_prgBankB = 1;
	_prgBankC = (uint8_t)~1;
	_prgBankD = (uint8_t)~0;
	_chrMask = 0;
	_chrMode = 0;
	_chrBankA = 0;
	_chrBankB = 1;
	_chrBankC = 2;
	_chrBankD = 3;
	_chrBankE = 4;
	_chrBankF = 5;
	_chrBankG = 6;
	_chrBankH = 7;
	_ppuLatch0 = 0;
	_ppuLatch1 = 0;
	_lreset = 0;
	_mmc1LoadRegister = 0;
	_mmc3Internal = 0;
	_mapper69Internal = 0;
	_mapper112Internal = 0;
	_mapper163Latch = 0;
	_mapper163R0 = 0;
	_mapper163R1 = 0;
	_mapper163R2 = 0;
	_mapper163R3 = 0;
	_mapper163R4 = 0;
	_mapper163R5 = 0;
	_mul1 = 0;
	_mul2 = 0;
	_mmc3IrqEnabled = 0;
	_mmc3IrqLatch = 0;
	_mmc3IrqCounter = 0;
	_mmc3IrqReload = 0;
	_mmc5IrqEnabled = 0;
	_mmc5IrqLine = 0;
	_mmc5IrqOut = 0;
	_mapper18IrqValue = 0;
	_mapper18IrqControl = 0;
	_mapper18IrqLatch = 0;
	_mapper65IrqEnabled = 0;
	_mapper65IrqValue = 0;
	_mapper65IrqLatch = 0;
	_mapper69IrqEnabled = 0;
	_mapper69CounterEnabled = 0;
	_mapper69IrqValue = 0;
	_vrc4IrqValue = 0;
	_vrc4IrqControl = 0;
	_vrc4IrqLatch = 0;
	_vrc4IrqPrescaler = 0;
	_vrc4IrqPrescalerCounter = 0;
	_vrc3IrqValue = 0;
	_vrc3IrqControl = 0;
	_vrc3IrqLatch = 0;
	_mapper42IrqEnabled = 0;
	_mapper42IrqValue = 0;
	_mapper83IrqEnabledLatch = 0;
	_mapper83IrqEnabled = 0;
	_mapper83IrqCounter = 0;
	_mapper90Xor = 0;
	_mapper67IrqEnabled = 0;
	_mapper67IrqLatch = 0;
	_mapper67IrqCounter = 0;
	_flashState = 0;
	_cfiMode = 0;
	_writeDelay = 0;

	Sync();
}

void Coolgirl::SaveBattery()
{
	if (!HasBattery()) return;
	if (_console->GetNesConfig().DisableFlashSaves) return;

	// 保存 WRAM 到 .sram 文件
	if (_wram.size() > 0) {
		_emu->GetBatteryManager()->SaveBattery(".sram", _wram.data(), (uint32_t)_wram.size());
	}

	// 保存 Flash 到 .fla 文件
	if (_saveFlash.size() > 0) {
		_emu->GetBatteryManager()->SaveBattery(".fla", _saveFlash.data(), (uint32_t)_saveFlash.size());
	}

	// 保留 IPS 保存作为兼容
	SaveRom(_orgPrgRom);
}

void Coolgirl::LoadBattery()
{
	if (!HasBattery()) return;
	if (_console->GetNesConfig().DisableFlashSaves) return;

	// 加载 WRAM
	if (_wram.size() > 0) {
		_emu->GetBatteryManager()->LoadBattery(".sram", _wram.data(), (uint32_t)_wram.size());
	}

	// 加载 Flash
	if (_saveFlash.size() > 0) {
		_emu->GetBatteryManager()->LoadBattery(".fla", _saveFlash.data(), (uint32_t)_saveFlash.size());
	}

	// 保留 IPS 加载作为兼容
	LoadRomPatch(_orgPrgRom);
}

void Coolgirl::Serialize(Serializer& s)
{
	BaseMapper::Serialize(s);

	SV(_wram);
	SV(_saveFlash);
	SV(_cfiData);

	SV(_sramEnabled); SV(_sramPage); SV(_canWriteChr); SV(_mapRomOn6000);
	SV(_flags); SV(_mapper); SV(_canWriteFlash); SV(_mirroring); SV(_fourScreen); SV(_lockout);
	SV(_prgBase); SV(_prgMask); SV(_prgMode); SV(_prgBank6000);
	SV(_prgBankA); SV(_prgBankB); SV(_prgBankC); SV(_prgBankD);
	SV(_chrMask); SV(_chrMode);
	SV(_chrBankA); SV(_chrBankB); SV(_chrBankC); SV(_chrBankD);
	SV(_chrBankE); SV(_chrBankF); SV(_chrBankG); SV(_chrBankH);
	SV(_tksMir[0]); SV(_tksMir[1]); SV(_tksMir[2]); SV(_tksMir[3]);
	SV(_tksMir[4]); SV(_tksMir[5]); SV(_tksMir[6]); SV(_tksMir[7]);
	SV(_a12Watcher);
	SV(_ppuLatch0); SV(_ppuLatch1); SV(_lreset);
	SV(_mmc1LoadRegister); SV(_mmc3Internal); SV(_mapper69Internal); SV(_mapper112Internal);
	SV(_mapper163Latch);
	SV(_mapper163R0); SV(_mapper163R1); SV(_mapper163R2); SV(_mapper163R3);
	SV(_mapper163R4); SV(_mapper163R5);
	SV(_mul1); SV(_mul2);
	SV(_mmc3IrqEnabled); SV(_mmc3IrqLatch); SV(_mmc3IrqCounter); SV(_mmc3IrqReload);
	SV(_mmc5IrqEnabled); SV(_mmc5IrqLine); SV(_mmc5IrqOut);
	SV(_mapper18IrqValue); SV(_mapper18IrqControl); SV(_mapper18IrqLatch);
	SV(_mapper65IrqEnabled); SV(_mapper65IrqValue); SV(_mapper65IrqLatch);
	SV(_mapper69IrqEnabled); SV(_mapper69CounterEnabled); SV(_mapper69IrqValue);
	SV(_vrc4IrqValue); SV(_vrc4IrqControl); SV(_vrc4IrqLatch);
	SV(_vrc4IrqPrescaler); SV(_vrc4IrqPrescalerCounter);
	SV(_vrc3IrqValue); SV(_vrc3IrqControl); SV(_vrc3IrqLatch);
	SV(_mapper42IrqEnabled); SV(_mapper42IrqValue);
	SV(_mapper83IrqEnabledLatch); SV(_mapper83IrqEnabled); SV(_mapper83IrqCounter);
	SV(_mapper90Xor);
	SV(_mapper67IrqEnabled); SV(_mapper67IrqLatch); SV(_mapper67IrqCounter);
	SV(_flashState);
	SV(_flashBufferA[0]); SV(_flashBufferA[1]); SV(_flashBufferA[2]); SV(_flashBufferA[3]); SV(_flashBufferA[4]);
	SV(_flashBufferA[5]); SV(_flashBufferA[6]); SV(_flashBufferA[7]); SV(_flashBufferA[8]); SV(_flashBufferA[9]);
	SV(_flashBufferV[0]); SV(_flashBufferV[1]); SV(_flashBufferV[2]); SV(_flashBufferV[3]); SV(_flashBufferV[4]);
	SV(_flashBufferV[5]); SV(_flashBufferV[6]); SV(_flashBufferV[7]); SV(_flashBufferV[8]); SV(_flashBufferV[9]);
	SV(_cfiMode);
	SV(_writeDelay);

	SerializeRomDiff(s, _orgPrgRom);
}

void Coolgirl::ProcessCpuClock()
{
	_writeDelay = 0;

	if (_vrc4IrqControl & 2) {
		_vrc4IrqPrescaler++;
		if ((!( _vrc4IrqPrescalerCounter & 2) && _vrc4IrqPrescaler == 114) ||
			((_vrc4IrqPrescalerCounter & 2) && _vrc4IrqPrescaler == 113)) {
			_vrc4IrqPrescaler = 0;
			_vrc4IrqPrescalerCounter++;
			if (_vrc4IrqPrescalerCounter == 0b11) _vrc4IrqPrescalerCounter = 0;
			_vrc4IrqValue++;
			if (_vrc4IrqValue == 0) {
				_console->GetCpu()->SetIrqSource(IRQSource::External);
				_vrc4IrqValue = _vrc4IrqLatch;
			}
		}
	}

	if (_vrc3IrqControl & 2) {
		if (_vrc3IrqControl & 4) {
			_vrc3IrqValue = (_vrc3IrqValue & 0xFF00) | ((_vrc3IrqValue + 1) & 0xFF);
			if ((_vrc3IrqValue & 0xFF) == 0) {
				_console->GetCpu()->SetIrqSource(IRQSource::External);
				_vrc3IrqValue = (_vrc3IrqValue & 0xFF00) | (_vrc3IrqLatch & 0xFF);
			}
		} else {
			_vrc3IrqValue++;
			if (_vrc3IrqValue == 0) {
				_console->GetCpu()->SetIrqSource(IRQSource::External);
				_vrc3IrqValue = _vrc3IrqLatch;
			}
		}
	}

	if (_mapper69CounterEnabled) {
		_mapper69IrqValue--;
		if (_mapper69IrqValue == 0xFFFF)
			_console->GetCpu()->SetIrqSource(IRQSource::External);
	}

	if (_mapper18IrqControl & 1) {
		uint8_t carry = (_mapper18IrqValue & 0x0F) - 1;
		_mapper18IrqValue = (_mapper18IrqValue & 0xFFF0) | (carry & 0x0F);
		carry = (carry >> 4) & 1;
		if (!(_mapper18IrqControl & 0b1000)) {
			carry = ((_mapper18IrqValue >> 4) & 0x0F) - carry;
			_mapper18IrqValue = (_mapper18IrqValue & 0xFF0F) | ((carry & 0x0F) << 4);
			carry = (carry >> 4) & 1;
		}
		if (!(_mapper18IrqControl & 0b1100)) {
			carry = ((_mapper18IrqValue >> 8) & 0x0F) - carry;
			_mapper18IrqValue = (_mapper18IrqValue & 0xF0FF) | ((carry & 0x0F) << 8);
			carry = (carry >> 4) & 1;
		}
		if (!(_mapper18IrqControl & 0b1110)) {
			carry = ((_mapper18IrqValue >> 12) & 0x0F) - carry;
			_mapper18IrqValue = (_mapper18IrqValue & 0x0FFF) | ((carry & 0x0F) << 12);
			carry = (carry >> 4) & 1;
		}
		if (carry)
			_console->GetCpu()->SetIrqSource(IRQSource::External);
	}

	if (_mapper65IrqEnabled) {
		if (_mapper65IrqValue != 0) {
			_mapper65IrqValue--;
			if (!_mapper65IrqValue)
				_console->GetCpu()->SetIrqSource(IRQSource::External);
		}
	}

	if (_mapper42IrqEnabled) {
		_mapper42IrqValue++;
		if (_mapper42IrqValue >> 15) _mapper42IrqValue = 0;
		if (((_mapper42IrqValue >> 13) & 0b11) == 0b11)
			_console->GetCpu()->SetIrqSource(IRQSource::External);
		else
			_console->GetCpu()->ClearIrqSource(IRQSource::External);
	}

	if (_mapper83IrqEnabled) {
		if (_mapper83IrqCounter == 0)
			_console->GetCpu()->SetIrqSource(IRQSource::External);
		_mapper83IrqCounter--;
	}

	if (_mapper67IrqEnabled) {
		_mapper67IrqCounter--;
		if (_mapper67IrqCounter == 0xFFFF) {
			_console->GetCpu()->SetIrqSource(IRQSource::External);
			_mapper67IrqEnabled = 0;
		}
	}
}

void Coolgirl::NotifyVramAddressChange(uint16_t addr)
{
	if (_mapper == CgMapper::MMC2_MMC4) {
		if ((addr >> 4) == 0xFD) { _ppuLatch0 = 0; SyncChr(); }
		if ((addr >> 4) == 0xFE) { _ppuLatch0 = 1; SyncChr(); }
		if ((addr >> 4) == 0x1FD) { _ppuLatch1 = 0; SyncChr(); }
		if ((addr >> 4) == 0x1FE) { _ppuLatch1 = 1; SyncChr(); }
	}

	if (_mapper == CgMapper::MMC3_MMC6) {
		A12StateChange change = _a12Watcher.UpdateVramAddress(addr, _console->GetMasterClock());
		if (change == A12StateChange::Rise) {
			ProcessScanlineCounter();
		}
	}

	if (_mapper == CgMapper::MMC3_MMC6 && (_flags & 1)) {
		SetMirroringType((_tksMir[(addr & 0x1FFF) >> 10] >> 7) ? MirroringType::ScreenBOnly : MirroringType::ScreenAOnly);
	}
}

void Coolgirl::ProcessScanlineCounter()
{
	if (_mmc3IrqReload || !_mmc3IrqCounter) {
		_mmc3IrqCounter = _mmc3IrqLatch;
		_mmc3IrqReload = 0;
	} else {
		_mmc3IrqCounter--;
	}
	if (!_mmc3IrqCounter && _mmc3IrqEnabled) {
		_console->GetCpu()->SetIrqSource(IRQSource::External);
	}

	if (_mapper == CgMapper::MMC5) {
		uint16_t scanline = _console->GetPpu()->GetCurrentScanline();
		if (_mmc5IrqLine == scanline + 1) {
			if (_mmc5IrqEnabled) {
				_console->GetCpu()->SetIrqSource(IRQSource::External);
				_mmc5IrqOut = 1;
			}
		}
	}

	if (_mapper == CgMapper::Mapper163) {
		uint16_t scanline = _console->GetPpu()->GetCurrentScanline();
		if (scanline == 239) {
			_mapper163Latch = 0;
			SyncChr();
		} else if (scanline == 127) {
			_mapper163Latch = 1;
			SyncChr();
		}
	}
}
