#include <GU800_GFX.h>

void GU800::drawPixel(int16_t x, int16_t y, uint16_t color) {
    if (x >= GU800_WIDTH || y >= GU800_HEIGHT || x < 0 || y < 0) { return; }

    uint8_t yByte = y / 8;
    uint8_t yBit = y % 8;

    uint8_t * data = & this->buffer[yByte][x];

    switch (color) {
        case BLACK:
            *data &= ~(1 << yBit);
            break;
        case INVERSE:
            *data ^= (1 << yBit);
            break;
        case WHITE:
        default:
            *data |= (1 << yBit);
            break;
    }
}

void GU800::display() {
    // Write buffer contents to display

    int yStart;         /* Starting Y-address in display buffer */
    bool page0, page1;

    // Determine which page is currently displayed
    switch (this->currentPage) {
        case 0:
            // Page 0: write to and display page 1
            this->currentPage = 1;
            yStart = GU800_HEIGHTBYTES;
            page0 = false;
            page1 = true;
            break;
        case 1:
        default:            
            // Page 1: write to and display page 0
            this->currentPage = 0;
            yStart = 0x0;
            page0 = true;
            page1 = false;
    }

    this->addrMode(true, false);    // Autoincrement X address, hold Y address
    for (int yByte = 0; yByte < GU800_HEIGHTBYTES; yByte++) {
        this->setXAddr(0);
        this->setYAddr(yByte + yStart);
        this->writeSPI(this->buffer[yByte], GU800_WIDTH, false);
    }

    this->mode(page0, page1, this->inverted);    // Display new page
}

void GU800::clearDisplay(bool display) {
    memset(this->buffer, 0, GU800_WIDTH * GU800_HEIGHTBYTES);
    if (display) this->display();
}

void GU800::invertDisplay(bool invert) {
    bool page0, page1;
    this->inverted = invert;
    switch (this->currentPage) {
        case 0:
            page0 = true;
            page1 = false;
            break;
        case 1:
        default:            
            page0 = false;
            page1 = true;
    }

    this->mode(page0, page1, this->inverted);
}

void GU800::displayOff(bool fade) {
    if (fade) {
        uint8_t oldBrightness = this->currentBrightness;
        while (this->currentBrightness > 0) {
            this->dim(this->currentBrightness - 1);
            delay(20);
        }
        this->currentBrightness = oldBrightness;
    }

    this->mode(false, false, false, false);
}

void GU800::displayOn(bool fade) {
    bool page0, page1;
    switch (this->currentPage) {
        case 0:
            page0 = true;
            page1 = false;
            break;
        case 1:
        default:            
            page0 = false;
            page1 = true;
    }

    this->mode(page0, page1, this->inverted, true);

    if (fade) {
        uint8_t oldBrightness = this->currentBrightness;
        this->dim(0);
        while (this->currentBrightness < oldBrightness) {
            this->dim(this->currentBrightness + 1);
            delay(20);
        }
    }
}

void GU800::begin() {
    SPI.begin();

    pinMode(dcPin, OUTPUT);

    csport    = portOutputRegister(digitalPinToPort(csPin));
    cspinmask = digitalPinToBitMask(csPin);
    dcport    = portOutputRegister(digitalPinToPort(dcPin));
    dcpinmask = digitalPinToBitMask(dcPin);

    SPI.beginTransaction(this->spi);
    SPI.endTransaction();

    if (this->resetPin != -1) {
        pinMode(resetPin, OUTPUT);
    }


    digitalWrite(csPin, HIGH);
    digitalWrite(dcPin, LOW);

    // Display initialization

    if (this->resetPin != -1) {
        // Hold /RESET low for 2ms to initialise display (only strictly required on
        // a cold poweron)
        digitalWrite(resetPin, LOW);
        delay(2);
        digitalWrite(resetPin, HIGH);
    }

    // Clear framebuffer and address counters
    this->clear(true, true, true);

    // Set all display regions to graphics mode
    for (int i = 0; i < 8; i++) {
        this->setArea(i, 0xFF);
    }

    // Switch on display, showing page 0
    this->mode(true, false, false);
}

void GU800::mode(bool layer0, bool layer1, bool inverse, bool on, GU800_Func func) {
    uint8_t l0 = 1 << 2;
    uint8_t l1 = 1 << 3; 
    
    uint8_t gs = 1 << 6;
    uint8_t grv = 1 << 4;
    uint8_t and_ = 1 << 3;
    uint8_t exor = 1 << 2;

    uint8_t cmd[] = { 0b00100000, 0 };

    if (layer0) cmd[0] |= l0;
    if (layer1) cmd[0] |= l1;

    if (on) cmd[1] |= gs;
    if (inverse) cmd[1] |= grv;

    switch (func) {
        case GU800_Func::AND:
            cmd[1] |= and_;
            break;
        case GU800_Func::XOR:
            cmd[1] |= exor;
            break;
        default:
            break;
    }

    this->writeSPI(cmd, 2, true);
}

void GU800::dim(uint8_t level) {
    level = constrain(level, 0, 0x0F);
    uint8_t cmd = 0b01000000 | level;
    this->currentBrightness = level;

    this->writeSPI(&cmd, 1, true);
}

void GU800::clear(bool layer0, bool layer1, bool addrs, bool wait) {
    uint8_t cmd = 0b01010010;
    if (layer0) cmd |= (1 << 2);
    if (layer1) cmd |= (1 << 3);
    if (addrs) cmd |= (1 << 0);

    this->writeSPI(&cmd, 1, true);

    if (wait) delay(1);
}

void GU800::setArea(uint8_t areaAddr, uint8_t mask) {
    uint8_t cmd = 0b01100010;
    areaAddr &= 0x7;

    this->writeSPI(cmd, true);
    this->writeSPI(areaAddr, true);
    this->writeSPI(mask, false);
}

void GU800::setXAddr(uint8_t addr) {
    uint8_t cmd[] = {0b01100100, addr};

    this->writeSPI(cmd, 2, true);
}

void GU800::setYAddr(uint8_t addr) {
    uint8_t cmd[] = {0b01100000, addr};

    this->writeSPI(cmd, 2, true);
}

void GU800::xShift(uint8_t offset) {
    uint8_t cmd[] = { 0b01110000, offset };
    this->writeSPI(cmd, 2, true);
}

void GU800::yShift(int8_t amount) {
    uint8_t cmd = 0b10110000;

    if (amount >= 0) {
        cmd |= (1 << 3);
    }

    switch (abs(amount)) {
        case 0:
            return;
        case 1:
            cmd |= (0b10 << 1);
            break;
        case 2:
            cmd |= (0b11 << 1);
            break;
        case 8:
            cmd |= (0b10 << 1);
            break;
        default:
            for (int i = 0; i < abs(amount); i++) {
                this->yShift(amount > 0 ? 1 : -1);
            }
    }
}

void GU800::addrMode(bool autoX, bool autoY) {
    uint8_t cmd = 0b10000000;

    if (autoY) cmd |= (1 << 3);
    if (autoX) cmd |= (1 << 2);

    this->writeSPI(&cmd, 1, true);
}

void GU800::getAddrs(uint8_t * xAddr, uint8_t * yAddr) {
    uint8_t cmd = 0b11010100;

    this->writeSPI(cmd, true);
    *xAddr = this->readSPI() & 0x7F;    // high bit of X address is undefined
    *yAddr = this->readSPI();
}

uint8_t GU800::readSPI() {
    *csport &= ~cspinmask;          // Assert /CS
    *dcport |= dcpinmask;           // DC should be high for reads

    SPI.beginTransaction(this->spi);

    uint8_t data = SPI.transfer(0);

    SPI.endTransaction();
    *csport |= cspinmask;

    return data;
}

size_t GU800::writeSPI(uint8_t data, bool isCommand) {
    *csport &= ~cspinmask;              // Assert /CS
    
    if (isCommand) {
        *dcport |= dcpinmask;           // Write command: DC high
    } else {
        *dcport &= ~dcpinmask;          // Write data: DC low
    }

    delayMicroseconds(1);               // Minimum 40nS between asserting /CS and starting transaction

    SPI.beginTransaction(this->spi);
    SPI.transfer(data);
    SPI.endTransaction();

    delayMicroseconds(2);               // Minimum 1.5uS between last clock and deasserting /CS
    *csport |= cspinmask;
    delayMicroseconds(1);               // Minimum 80nS between deasserting /CS and next transaction

    return 1;
}

size_t GU800::writeSPI(uint8_t * buf, size_t count, bool isCommand) {
    // GU800 requires that we deassert and reassert /CS after every byte, so no
    // gain in doing batch transfers :(

    for (int i = 0; i < count; i++) {
        this->writeSPI(buf[i], isCommand);
    }

    return count;
}

