include "mongo_blocks.h"

void iniciar_superbloque(FILE* archivo) { // No se destruye bitarray
    uint32_t block_size = 64; // Bytes
    uint32_t size = 64;
    void* puntero_a_bits = malloc(8);
    t_bitarray* bitmap = bitarray_create_with_mode(puntero_a_bits, 8, LSB_FIRST); 

    for(int i; i < 64; i++) {
        bitarray_clean_bit(bitmap, i);
    }

    fwrite("BITE_SIZE=", strlen("BITE_SIZE="), 1, archivo);
    fwrite(block_size, sizeof(uint32_t), 1, archivo);

    fwrite("BLOCKS=", strlen("BLOCKS="), 1, archivo);
    fwrite(size, sizeof(uint32_t), 1, archivo);

    fwrite(bitmap, sizeof(bitmap), 1, archivo);
}

void iniciar_blocks(FILE* archivo, int filedescriptor) {
    fseek(archivos.superbloque, strlen("BITE_SIZE="), SEEK_SET);
    uint32_t* block_size;
    fread(block_size, sizeof(uint32_t), 1, archivos.superbloque);

    fseek(archivos.superbloque, strlen("BLOCKS=", SEEK_CUR));
    uint32_t* size;
    fread(size, sizeof(uint32_t), 1, archivos.superbloque);

    void* mapa = mmap(NULL, block_size * size, PROT_NONE, MAP_SHARED, filedescriptor, 0); // Revisar flags

    memcpy(archivos.mapa_blocks, mapa, sizeof(mapa)); // Actualizar struct
}
