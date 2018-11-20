#include <Adafruit_GFX.h>
#include <SPI.h>

#define BLACK 0
#define WHITE 1
#define INVERSE 2

#define GU800_WIDTH 128
#define GU800_HEIGHT 64

#define GU800_HEIGHTBYTES (GU800_HEIGHT / 8)

enum class GU800_Func {
    AND = 0b10,
    XOR = 0b01,
    OR  = 0b00
};

enum class GU800_ShiftDir {
    UP,
    DOWN
};

class GU800 : public Adafruit_GFX {
    public:
    GU800(int csPin, int dcPin, int resetPin=-1) : Adafruit_GFX(GU800_WIDTH, GU800_HEIGHT), csPin(csPin), dcPin(dcPin), resetPin(resetPin) {};

    virtual void drawPixel(int16_t x, int16_t y, uint16_t color) override;

    void begin();                           // Initialize display (with /RESET if pin defined)
    void display();                         // Write framebuffer to offscreen page, then flip page
    void clearDisplay(bool display=false);  // Clear framebuffer; call with display=true to also clear displayed image
    virtual void invertDisplay(bool invert) override;  // Enable/disable inverse video
    void dim(uint8_t level);                // Dim the display (0 is dimmest, 15 is brightest)
    void displayOff(bool fade=true);        // Turn the display off (image is preserved)
    void displayOn(bool fade=true);         // Turn the display on
    
    private:
    /* These map more-or-less directly onto controller commands; private because
    messing with these could put the display into a state the library doesn't
    understand */
    void mode(bool layer0, bool layer1, bool inverse=false, bool on=true, GU800_Func func=GU800_Func::OR);
    void clear(bool layer0, bool layer1, bool addrs, bool wait=true);;
    void setArea(uint8_t areaAddr, uint8_t mask);

    void setXAddr(uint8_t addr);
    void setYAddr(uint8_t addr);

    void xShift(uint8_t offset);
    void yShift(int8_t amount);

    void addrMode(bool autoX, bool autoY);

    void getAddrs(uint8_t * xAddr, uint8_t * yAddr);

    uint8_t readSPI();
    size_t writeSPI(uint8_t * buf, size_t count, bool isCommand=true);
    size_t writeSPI(uint8_t data, bool isCommand=true);

    uint8_t currentPage = 0;            /* Current page being displayed; 0 or 1 */
    bool inverted = false;
    uint8_t currentBrightness = 15;

    const int csPin, dcPin, resetPin;
    volatile uint8_t *csport, *dcport;
    uint8_t cspinmask, dcpinmask;
    const SPISettings spi = SPISettings(5000000, MSBFIRST, SPI_MODE3);

    uint8_t buffer[GU800_HEIGHTBYTES][GU800_WIDTH];
};