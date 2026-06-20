// Force 64-bit freestanding entry mapping
void kernel_main(void) {
    // 0xB8000 color video graphics cell pointer
    unsigned short *video_memory = (unsigned short *)0xB8000;
    
    // Pure display message array layout
    const char *message = "SaarOS: 64-Bit Microkernel Canvas Initialized Successfully!";
    
    // Clear screen with a professional dark blue background canvas
    for (int i = 0; i < 80 * 25; i++) {
        video_memory[i] = (0x1F << 8) | ' '; // 0x1F = Bright White text on Blue background
    }
    
    // Print string starting from row 2, column 3 for a clean UI look
    int offset = (2 * 80) + 3; 
    int i = 0;
    
    while (message[i] != '\0') {
        // Character byte + Color code merge sequence
        video_memory[offset + i] = (0x1F << 8) | message[i];
        i++;
    }
    
    // Screen margin border execution (Blank Canvas aesthetic look)
    for(int col = 0; col < 80; col++) {
        video_memory[col] = (0x9F << 8) | '='; // Top border line
        video_memory[(24 * 80) + col] = (0x9F << 8) | '='; // Bottom bar
    }
}