//@echo off
//REM Cambia el nombre del archivo según tu archivo fuente
//set SOURCE=vox.c
//set OUTPUT=vox.exe

//REM Cambia al directorio donde está tu archivo fuente
//cd /d c:\voxelito

//REM Compila el programa usando GCC
//gcc %SOURCE% -o %OUTPUT% -lgdi32 -lwinmm

//REM Verifica si la compilación fue exitosa
//if %ERRORLEVEL% neq 0 (
//    echo Error durante la compilación.
//    pause
//    exit /b
//)

//echo Compilación exitosa. Ejecutando el programa...
//start %OUTPUT%

//para agregar niebla, multiplicar distancia z x pixels :)
//corregir movimiento relativo a cam.local :)
//añadir indice de voxels(obj) + diccionario de tipos + buscador inarray :)
//implementar funciones para dibujar prismas :)
//implementar funciones para esferas :)
//implementar brush, destruct, painter, puntero. :)
//implementar chunksplitter, chunkloader, chunkleaner :(
//implementar world, collections, objects :(
//implementar luces, global, radiancevox, reflection, refraction, material, diff. CASI :)
//impelementar funciones de fractal noise(musgrave), voronoi(cellular), wave(armonics), cell noise CASI :)
//implementar function blocks :V

#include <windows.h>  // Incluye la API de Windows
#include <stdlib.h>   // Incluye funciones estándar como rand y srand
#include <time.h>     // Incluye funciones para manejo de tiempo
#include <stdio.h>    // Incluye funciones de entrada/salida estándar
#include <math.h>     // Incluye funciones matemáticas
#include <stdbool.h>  // Incluye booleanos
#include <mmsystem.h> // System

//CANVAS
#define WIDTH 80	  // Ancho de la grilla de píxeles 64
#define HEIGHT 40	  // Altura de la grilla de píxeles 32
#define SCALE 8	  // Escala para agrandar los píxeles

//GLOBAL
int seed = 312;
float gravityInt = -0.08;
bool gravityActivated = false;
float jumpspeed = 0.5;
int frameCount = 0;

//FOG
int fogValue = 0.1;
float fogInt = 0.01;
int fogStart = 8;

//OUTLINE
float outline = 0.0;
float borderTolerance = 0.55;
float borderIntensity = 0.8; //multiplicador por decimal o sea digamos oscurece
float preMargin = 0.45;

//RAY
#define EPSILON 0.001f //tol
int isActive = 1;
int drawVox = 0;
int deleteVox = 0;
float stepsMult = 6.00; //precision del dda
float maxZ = 1.0;		//multiplicador de zdist x plano de cam para ddd

//CAM
float speed = 0.10;		// Multiplicador de velocidad de pos
float rotspeed = 5.00;	// Multiplicador de velocidad de rot
float dof = 20.00;		// Distancia focal/plano
float aperture = 0.80;	//multiplicador de apertura

float originX = 0.00;	// Posición x del origen de la cámara
float originY = 0.00;	// Posición y del origen de la cámara
float originZ = 0.00;	// Posición z del origen de la cámara

float endX = 0.00;	// Posición x del final de la cámara
float endY = 0.00;	// Posición y del final de la cámara
float endZ = 0.00;	// Posición z del final de la cámara

float relX = 0.00;	// Vector relativo (end-origin) x
float relY = 0.00;	// Vector relativo (end-origin) y
float relZ = 0.00;	// Vector relativo (end-origin) z (DOF)

float angleY = 0.00;	// Ángulo de rotación alrededor del eje Y (en grados)
float angleX = 0.00;	// Ángulo de rotación alrededor del eje X (en grados)

float radY = 0.00;	// en radianes
float radX = 0.00;	// en radianes

//MOUSE
POINT lastMousePos = { 0, 0 }; // Guardamos la posición anterior del mouse
bool isMouseInitialized = false;
short wDelta = 0;
float previousFreeX = 0.0;
float previousFreeY = 0.0;
float freeX = 0.0f;
float freeY = 0.0f;

//POINTER
int type = 0;		//tipo de voxel
int pointerType = 0; //type del brush
int lit = 1;
bool isCenter = false;
bool notEmpty = false;
bool eraseable = false;
bool isVox = false;

//MISC BOOLS
bool isGlass = false; //detecta si es vidrio
bool truGlass = false; //el rayo esta atravesando vidrio
bool consola = false;
bool grid = false;
bool normal = false;
bool isEntity = false;
bool lightNear = false;

//COLLISION
bool colisionF = false;
bool colisionB = false;
bool colisionL = false;
bool colisionR = false;
bool colisionU = false;
bool colisionD = false;

//STRUCTURES

///////////////////////////////////COLOR/////////////////////////////////
typedef struct {
    unsigned char r, g, b;
	char name[16];
} Color;

Color colorDictionary[] = {
    {0, 0, 0, "VOID/RADIANCE"},
    {255, 0, 0, "RED/EM_LAVA"},
    {0, 255, 0, "GREEN/WAVE"},
    {128, 64, 0, "BROWN/BRICK"},
    {0, 255, 255, "CYAN/WATER"},
    {92, 64, 0, "BROWN"},
    {255, 255, 0, "YELLOW"},
    {255, 255, 255, "WHITE/LIGHT"},
    {128, 128, 128, "GRAY/CHECKER"},
    {128, 128, 128, "GRAY/CELLS"},
    {0, 128, 0, "TISSUE"},
    {0, 0, 128, "DARK_BLUE"},
    {128, 128, 0, "PLANT"},
    {128, 0, 128, "PURPLE"},
    {0, 128, 128, "TEAL"},
    {192, 192, 192, "LIGHT_GRAY"},
    {0, 255, 255, "GLASS_CYAN"},
    {0, 0, 255, "BLUE"}
};

/////////////////////////////////VECTOR//////////////////////////////////
typedef struct {
    float x, y, z;
} Vector;

//////////////////////////////////QUATERNIONS////////////////////////////
typedef struct {
    float w, x, y, z;
} Quaternion;

Quaternion qY = {1, 0, 0, 0};
Quaternion qX = {1, 0, 0, 0};

/////////////////////////////////////VOXEL//////////////////////////////
typedef struct Voxel {
    int x, y, z;         // Coordenadas del voxel
    int type;            // Datos adicionales, podrías cambiarlo según necesidades
	int lit;			 // Nivel de iluminacion, por defecto 1
    struct Voxel *next;  // Apuntador al siguiente voxel en la lista
} Voxel;

Voxel pointerVoxel = {0,0,0};
Voxel toDeleteVoxel = {0,0,0};
Voxel v1 = {1,1,1,6};
Voxel previousVoxel = {0,0,0};
Voxel* voxelList = NULL;

//////////////////////////////////ENTITIES//////////////////////////////

#define ENTITY_MAP_SIZE 8

typedef struct {
    float x, y, z;
    Color col;
	int map[ENTITY_MAP_SIZE][ENTITY_MAP_SIZE];
} Entity;

float entSpeed = 500.0f;
float deltaTime = 0.50f;

/////////////////////////////////PROCEDURAL TEXTURES////////////////////

bool gridPattern3D(float x, float y, float z, float cellSize) {
    // Alinear las coordenadas al centro de cada celda
    float alignedX = fmodf(x - floorf(x / cellSize) * cellSize, cellSize);
    float alignedY = fmodf(y - floorf(y / cellSize) * cellSize, cellSize);
    float alignedZ = fmodf(z - floorf(z / cellSize) * cellSize, cellSize);

    bool xInCell = alignedX < cellSize / 2;
    bool yInCell = alignedY < cellSize / 2;
    bool zInCell = alignedZ < cellSize / 2;
    return xInCell ^ yInCell ^ zInCell;  // Alterna en 3D como una cuadrícula de tablero
}

float voronoiPattern3D(float x, float y, float z, float cellSize) {
    // Coordenadas relativas de la celda
    int ix = (int)(x / cellSize);
    int iy = (int)(y / cellSize);
    int iz = (int)(z / cellSize);

    // Genera una pseudo-aleatoriedad con los índices de la celda
    float randomVal = fmodf(sin(ix + iy * 57 + iz * 133) * 43758.5453, 1.0f);

    // El patrón será '0' si está cerca del centro de la celda, y '1' si está lejos
    return (fmodf(randomVal * 10.0f, 1.0f) < 0.5f) ? 1.0f : 0.0f;
}

float animVoronoiPattern3D(float x, float y, float z, float cellSize) {
    // Coordenadas relativas de la celda
    int ix = (int)(x / cellSize);
    int iy = (int)(y / cellSize) + (frameCount*0.5f);
    int iz = (int)(z / cellSize);

    // Genera una pseudo-aleatoriedad con los índices de la celda
    float randomVal = fmodf(sin(ix + iy * 57 + iz * 133) * 43758.5453, 1.0f);

    // El patrón será '0' si está cerca del centro de la celda, y '1' si está lejos
    return (fmodf(randomVal * 10.0f, 1.0f) < 0.5f) ? 1.0f : 0.0f;
}

float fluidPattern3D(float x, float y, float z, float time, float frequency, float amplitude) {
    // Combinación de varias funciones seno para obtener una forma más fluida
    float waveX = amplitude * sinf(x * frequency + time);
    float waveY = amplitude * cosf(y * frequency + time);
    float waveZ = amplitude * sinf(z * frequency + time);

    // El resultado será una combinación de las tres ondas
    return (waveX + waveY + waveZ) / 3.0f;
}

float meshPattern3D(float x, float y, float z, float frequency, float amplitude, float threshold) {
    // Modulamos las posiciones en cada eje por una onda sinusoide.
    float waveX = sinf(x * frequency) * amplitude;
    float waveY = cosf(y * frequency) * amplitude;
    float waveZ = sinf(z * frequency) * amplitude;

    // Sumamos las ondas de los tres ejes
    float value = waveX + waveY + waveZ;

    // Aplica un umbral para hacer que el patrón se vea más como una malla
    if (value > threshold) {
        return 1.0f;  // Parte de la malla
    } else {
        return 0.0f;  // Espacio vacío
    }
}

bool stripePattern3D(float x, float y, float z, float stripeWidth) {
    // Calcular las coordenadas globales del voxel
    float globalX = floorf(x / stripeWidth);
    float globalY = floorf(y / stripeWidth);
    float globalZ = floorf(z / stripeWidth);
    
    // Alternar el patrón en función de las coordenadas globales
    bool xStripe = fmodf(globalX, 2.0f) < 1.0f;
    bool zStripe = fmodf(globalZ, 2.0f) < 1.0f;
    
    // Usamos XOR para alternar en ambos ejes sin distorsionar
    return xStripe ^ zStripe;
}

bool dotPattern3D(float x, float y, float z, float spacing) {
    // Alinear las coordenadas a la celda de tamaño `spacing`
    float alignedX = fmodf(x - floorf(x / spacing) * spacing, spacing);
    float alignedY = fmodf(y - floorf(y / spacing) * spacing, spacing);
    float alignedZ = fmodf(z - floorf(z / spacing) * spacing, spacing);

    // Verificar si el punto está dentro de la celda en cada eje
    return (alignedX < spacing / 2) || (alignedY < spacing / 2) || (alignedZ < spacing / 2);
}

int cloudPattern3D(float x, float y, float z, float density) {
    // Usa una función pseudoaleatoria basada en la posición para crear el patrón
    float randValue = fmodf(sinf(x * 12.9898f + y * 78.233f + z * 34.654f) * 43758.5453f, 1.0f);
    
    // Si el valor aleatorio está por debajo de la densidad, el vóxel forma parte de las "nubes"
    if (randValue < density) {
        return 1;  // Vóxel dentro de la nube
    }

    return 0;  // Fuera de la nube
}

int animCloudPattern3D(float x, float y, float z, float density) {
    // Usa una función pseudoaleatoria basada en la posición para crear el patrón
    float randValue = fmodf(sinf(x * 12.9898f + y * 78.233f + z * 34.654f) * 43758.5453f, 1.0f) + frameCount;
    
    // Si el valor aleatorio está por debajo de la densidad, el vóxel forma parte de las "nubes"
    if (randValue < density) {
        return 1;  // Vóxel dentro de la nube
    }

    return 0;  // Fuera de la nube
}

int diagonalStripePattern3D(float x, float y, float z, float stripeWidth) {
    // Calculamos las posiciones dentro de cada celda del vóxel
    float localX = fmodf(x, stripeWidth);
    float localY = fmodf(y, stripeWidth);
    float localZ = fmodf(z, stripeWidth);
    
    // Creamos rayas paralelas en el plano XY
    int stripeXY = (fmodf(x + y, stripeWidth) < stripeWidth / 2);
    
    // Creamos rayas paralelas en el plano XZ
    int stripeXZ = (fmodf(x + z, stripeWidth) < stripeWidth / 2);
    
    // Creamos rayas paralelas en el plano YZ
    int stripeYZ = (fmodf(y + z, stripeWidth) < stripeWidth / 2);

    // Devolvemos 1 si cualquiera de las caras tiene una raya diagonal
    return stripeXY || stripeXZ || stripeYZ;
}

/////////////////CHARACTER BITMAP TEXTURES///////////////////////////////

// Matriz 8x8 de bits para el outline (1 indica outline, 0 indica fondo)
int brickMap[8][8] = {
    {1, 1, 1, 1, 1, 1, 1, 1},
    {0, 0, 0, 0, 0, 0, 0, 1},
    {0, 0, 0, 0, 0, 0, 0, 1},
    {0, 0, 0, 0, 0, 0, 0, 1},
    {1, 1, 1, 1, 1, 1, 1, 1},
    {0, 0, 0, 0, 1, 0, 0, 0},
    {0, 0, 0, 0, 1, 0, 0, 0},
    {0, 0, 0, 0, 1, 0, 0, 0}
};

int sandMap[8][8] = {
    {0, 1, 0, 0, 0, 0, 0, 1},
    {0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 1, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 1},
    {0, 0, 1, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 1, 0, 0, 0},
    {0, 1, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0}
};

int metalMap[8][8] = {
    {0, 0, 0, 0, 0, 0, 0, 0},
    {0, 1, 0, 1, 0, 1, 0, 1},
    {0, 0, 0, 0, 0, 0, 0, 0},
    {0, 1, 0, 1, 0, 1, 0, 1},
    {0, 0, 0, 0, 0, 0, 0, 0},
    {0, 1, 0, 1, 0, 1, 0, 1},
    {0, 0, 0, 0, 0, 0, 0, 0},
    {0, 1, 0, 1, 0, 1, 0, 1}
};

int woodMap[8][8] = {
    {0, 1, 0, 1, 0, 1, 0, 1},
    {0, 1, 0, 1, 0, 1, 0, 1},
    {0, 1, 0, 1, 0, 1, 0, 1},
    {0, 1, 0, 1, 0, 1, 0, 1},
    {0, 1, 0, 1, 0, 1, 0, 1},
    {0, 1, 0, 1, 0, 1, 0, 1},
    {0, 1, 0, 1, 0, 1, 0, 1},
    {0, 1, 0, 1, 0, 1, 0, 1}
};

int frameMap[8][8] = {
    {1, 1, 1, 1, 1, 1, 1, 1},
    {1, 0, 0, 0, 0, 0, 0, 1},
    {1, 0, 0, 0, 0, 0, 0, 1},
    {1, 0, 0, 0, 0, 0, 0, 1},
    {1, 0, 0, 0, 0, 0, 0, 1},
    {1, 0, 0, 0, 0, 0, 0, 1},
    {1, 0, 0, 0, 0, 0, 0, 1},
    {1, 1, 1, 1, 1, 1, 1, 1}
};

const int crossMap[ENTITY_MAP_SIZE][ENTITY_MAP_SIZE] = {
    {0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 1, 1, 1, 1, 0, 0},
    {0, 0, 1, 1, 1, 1, 0, 0},
    {0, 0, 1, 1, 1, 1, 0, 0},
    {0, 0, 1, 1, 1, 1, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0}
};

// Inicializa la entidad
Entity ent = {0.2f, -0.9f, 3.1f, {255, 0, 0}};

// Copia el mapa a la entidad después de inicializarla
void initializeEntityMap(Entity *ent) {
    for (int i = 0; i < ENTITY_MAP_SIZE; i++) {
        for (int j = 0; j < ENTITY_MAP_SIZE; j++) {
            ent->map[i][j] = crossMap[ENTITY_MAP_SIZE - 1 - i][j];
        }
    }
}

////////////////////////BITMAP & WINDOW MANAGEMENT///////////////

// Estructura para almacenar los datos del bitmap

BITMAPINFO bi; // Información del bitmap
BYTE pixels[HEIGHT][WIDTH * 4]; // Matriz de bytes p/ color de cada píxel (4 bytes-BGRA)

// Función de manejo de mensajes de la ventana
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    //switch (uMsg) {	
	//case WM_MOUSEWHEEL: {

	//	zDelta += GET_WHEEL_DELTA_WPARAM(wParam);
	//}
	//}
    if (uMsg == WM_DESTROY) { // Si se recibe un mensaje de destruir la ventana
        PostQuitMessage(0);    // Cierra la aplicación
        return 0;
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam); // Llama al procedimiento por defecto
}

//////////////////////ADJUSTMENTS////////////////////////////////

void captureWheel(){
    wDelta += (GetAsyncKeyState(0x6B) ? 1 : 0.0f);
    wDelta -= (GetAsyncKeyState(0x6D) ? 1 : 0.0f);
    pointerType = wDelta;

    // Asegurarse de que el índice esté dentro de los límites del array colorDictionary
    int numColors = sizeof(colorDictionary) / sizeof(colorDictionary[0]);
    if (pointerType >= 0 && pointerType < numColors) {
        printf("pointer: %d, color: %s\n", pointerType, colorDictionary[pointerType].name);
    } else {
        printf("pointer: %d, color: Out of Range\n", pointerType);
    }
}

void centrarVentana(HWND hwnd) {
    // Obtener el tamaño de la pantalla
    RECT rect;
    GetClientRect(hwnd, &rect);
    int width = rect.right - rect.left;
    int height = rect.bottom - rect.top;

    // Obtener el tamaño de la pantalla
    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);

    // Calcular la posición de la ventana
    int xPos = (screenWidth - width) / 2;
    int yPos = (screenHeight - height) / 2;

    width += 50;  // Ajusta este valor según lo necesario
    height += 50; // Ajusta este valor según lo necesario

    // Mover la ventana a la posición calculada
    MoveWindow(hwnd, xPos, yPos, width, height, TRUE);
}

void infiniteWrap(){
    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);

    POINT cursorPos;
    GetCursorPos(&cursorPos);

    // Calcular el cambio en x e y desde el centro de la pantalla
    int deltaX = cursorPos.x - (screenWidth / 2);
    int deltaY = cursorPos.y - (screenHeight / 2);

    // Solo actualizar freeX y freeY si el cursor realmente se ha movido
    if (deltaX != 0 || deltaY != 0) {
        // Actualizar ángulos basados en el cambio del cursor
        angleX += deltaY * 0.5f;
        angleY += deltaX * 0.5f;

        // Sumar el cambio del cursor a freeX y freeY
        freeX += deltaX;
        freeY += deltaY;

        // Imprimir valores actuales
        //printf("Cursor X: %d, Cursor Y: %d\n", cursorPos.x, cursorPos.y);
        //printf("Angle X: %.2f, Angle Y: %.2f\n", angleX, angleY);
        //printf("Free X: %.2f, Free Y: %.2f\n", freeX, freeY);

        // Si el cursor alcanzó los límites, enviarlo al centro de la pantalla
        SetCursorPos(screenWidth / 2, screenHeight / 2);
    }
}

//////////////////////SOME MISC MECHANICS////////////////////////

void playSound(const char *file) {
    PlaySound(file, NULL, SND_FILENAME | SND_ASYNC);
}

void gravity(){
	originY += gravityInt;
}

void followTarget() {
    // Calcula la diferencia entre el seguidor y el objetivo
    float deltaX = originX - ent.x;
    float deltaY = originY - (ent.y);
    float deltaZ = originZ - ent.z;
    
    // Calcula la distancia actual entre ambos
    float distance = sqrt(deltaX * deltaX + deltaY * deltaY + deltaZ * deltaZ);
    
    // Verifica que la distancia no sea cero para evitar divisiones por cero
    if (distance > 0.0f) {
        // Normaliza el vector de dirección
        float directionX = deltaX / distance;
        float directionY = deltaY / distance;
        float directionZ = deltaZ / distance;
        
        // Mueve el seguidor en la dirección del objetivo usando la velocidad
        ent.x += directionX * speed * deltaTime;
        ent.y += directionY * speed * deltaTime;
        ent.z += directionZ * speed * deltaTime;
        
        // Evita que el seguidor se sobrepase del objetivo si está muy cerca
        //if (fabs(deltaX) < entSpeed) ent.x = originX;
        //if (fabs(deltaY) < entSpeed) ent.y = originY;
        //if (fabs(deltaZ) < entSpeed) ent.z = originZ;
    }
}

/////////////////////////VOXEL FUNCTIONS/////////////////////////

void makeVoxel(){
		Voxel v1;         // Crea un voxel
		v1.x = 1;         // Asigna la coordenada x
		v1.y = 2;         // Asigna la coordenada y
		v1.z = 3;         // Asigna la coordenada z
		v1.type = 1;      // Asigna un valor al dato
}

void guardarVoxeles(const char* archivo) {
    FILE* file = fopen(archivo, "wb");
    if (file == NULL) {
        perror("No se pudo abrir el archivo para guardar");
        return;
    }

    Voxel* actual = voxelList;
    while (actual != NULL) {
        fwrite(&(actual->x), sizeof(int), 1, file);
        fwrite(&(actual->y), sizeof(int), 1, file);
        fwrite(&(actual->z), sizeof(int), 1, file);
        fwrite(&(actual->type), sizeof(int), 1, file);
		fwrite(&(actual->lit), sizeof(int), 1, file);
        actual = actual->next;
    }

    fclose(file);
}

void cargarVoxeles(const char* archivo) {
    FILE* file = fopen(archivo, "rb");
    if (file == NULL) {
        perror("No se pudo abrir el archivo para cargar");
        return;
    }

    Voxel* nuevoVoxel;
    while (!feof(file)) {
        nuevoVoxel = (Voxel*)malloc(sizeof(Voxel));
        if (fread(&(nuevoVoxel->x), sizeof(int), 1, file) != 1) break;
        fread(&(nuevoVoxel->y), sizeof(int), 1, file);
        fread(&(nuevoVoxel->z), sizeof(int), 1, file);
        fread(&(nuevoVoxel->type), sizeof(int), 1, file);
		fread(&(nuevoVoxel->lit), sizeof(int), 1, file);
        nuevoVoxel->next = voxelList;
        voxelList = nuevoVoxel;
    }

    fclose(file);
}

void agregarVoxel(int x, int y, int z, int type, int lit) {
    // Buscar en la lista si ya existe un voxel con las mismas coordenadas
    Voxel* actual = voxelList;
    while (actual != NULL) {
        if (actual->x == x && actual->y == y && actual->z == z) {
            // Si existe, actualiza su tipo y sale de la función
            actual->type = type;
			actual->lit = lit;
            return;
        }
        actual = actual->next;
    }

    // Si no existe, crea un nuevo voxel y lo inserta al inicio de la lista
    Voxel* nuevoVoxel = (Voxel*)malloc(sizeof(Voxel));
    nuevoVoxel->x = x;
    nuevoVoxel->y = y;
    nuevoVoxel->z = z;
    nuevoVoxel->type = type;
	nuevoVoxel->lit = lit;
    nuevoVoxel->next = voxelList;  // Inserta al inicio de la lista
    voxelList = nuevoVoxel;
	
	guardarVoxeles("voxeles.bin");
}

void borrarVoxel(int x, int y, int z) {
    Voxel* actual = voxelList;
    Voxel* anterior = NULL;

    // Recorrer la lista para encontrar el voxel
    while (actual != NULL) {
        if (actual->x == x && actual->y == y && actual->z == z) {
            // Si el voxel a borrar es el primero en la lista
            if (anterior == NULL) {
                voxelList = actual->next; // Mover el inicio de la lista
            } else {
                anterior->next = actual->next; // Desvincular el voxel de la lista
            }

            // Liberar la memoria del voxel
            free(actual);
            return; // Salir después de borrar
        }
        anterior = actual;
        actual = actual->next;
    }
}

int buscarVoxel(int x, int y, int z) {
    Voxel* actual = voxelList;
    while (actual != NULL) {
        if (actual->x == x && actual->y == y && actual->z == z) {
            return actual->type;
        }
        actual = actual->next;
    }
    return 0;  // Tipo por defecto si el vóxel no se encuentra
}

int buscarVoxelLit(int x, int y, int z) {
    Voxel* actual = voxelList;
    while (actual != NULL) {
        if (actual->x == x && actual->y == y && actual->z == z) {
            return actual->lit;
        }
        actual = actual->next;
    }
    return 0;  // Tipo por defecto si el vóxel no se encuentra
}

void inicializarVoxeles() {
	
	int xx = 7;
	int yy = 0;
	int zz = 7;
	
	for (int x = 0; x <= xx; x++) {
		for (int y = 0; y <= yy; y++) {
			for (int z = 0; z <= zz; z++) {
				
			agregarVoxel(x-3, y-2, z-3, (rand() % 18)+1, 1);
			
			}
		}
	}
}

void dibujarEsfera() {
	
	int size = 3; //size impar
    int cx = size / 2;
    int cy = size / 2;
    int cz = size / 2;
    int r = size / 2;

    for (int x = 0; x < size; x++) {
        for (int y = 0; y < size; y++) {
            for (int z = 0; z < size; z++) {
                // Calcular la distancia al centro
                int dx = x - cx;
                int dy = y - cy;
                int dz = z - cz;
                
                // Verificar si el punto está dentro de la esfera
                if (dx * dx + dy * dy + dz * dz <= r * r) {
					
					agregarVoxel(x-(size/2), y, z+5, (rand() % 16)+1, 1);
                }
            }
        }
    }
}

void liberarVoxeles() {
    Voxel* actual = voxelList;
    while (actual != NULL) {
        Voxel* siguiente = actual->next;
        free(actual);
        actual = siguiente;
    }
}

/////////////////////////////LIGHTING////////////////////////////

void fillRadiance(){
			
	int x = pointerVoxel.x;
	int y = pointerVoxel.y;
	int z = pointerVoxel.z;
	
	Voxel litVoxel = {x, y, z};	
	int type = 0;
	
	litVoxel.x = x;
	litVoxel.y = y + 1;
	litVoxel.z = z;	
	type = buscarVoxel(litVoxel.x, litVoxel.y, litVoxel.z);			
	if (type == 0){
		agregarVoxel(litVoxel.x, litVoxel.y, litVoxel.z, 0, 2);
		}
		
	litVoxel.x = x;
	litVoxel.y = y - 1;
	litVoxel.z = z;			
	type = buscarVoxel(litVoxel.x, litVoxel.y, litVoxel.z);
	if (type == 0){
		agregarVoxel(litVoxel.x, litVoxel.y, litVoxel.z, 0, 2);
	}
	
	litVoxel.x = x + 1;
	litVoxel.y = y;
	litVoxel.z = z;			
	type = buscarVoxel(litVoxel.x, litVoxel.y, litVoxel.z);
	if (type == 0){
		agregarVoxel(litVoxel.x, litVoxel.y, litVoxel.z, 0, 2);
	}
			
	litVoxel.x = x - 1;
	litVoxel.y = y;
	litVoxel.z = z;			
	type = buscarVoxel(litVoxel.x, litVoxel.y, litVoxel.z);
	if (type == 0){
		agregarVoxel(litVoxel.x, litVoxel.y, litVoxel.z, 0, 2);
	}
			
	litVoxel.x = x;
	litVoxel.y = y;
	litVoxel.z = z + 1;			
	type = buscarVoxel(litVoxel.x, litVoxel.y, litVoxel.z);
	if (type == 0){
		agregarVoxel(litVoxel.x, litVoxel.y, litVoxel.z, 0, 2);
	}
	
	litVoxel.x = x;
	litVoxel.y = y;
	litVoxel.z = z - 1;				
	type = buscarVoxel(litVoxel.x, litVoxel.y, litVoxel.z);
	if (type == 0){
		agregarVoxel(litVoxel.x, litVoxel.y, litVoxel.z, 0, 2);
	}	
}

void emptyRadiance(){
			
	int x = toDeleteVoxel.x;
	int y = toDeleteVoxel.y;
	int z = toDeleteVoxel.z;
	
	Voxel litVoxel = {x, y, z};	
	int type = 0;
	
	litVoxel.x = x;
	litVoxel.y = y + 1;
	litVoxel.z = z;	
	type = buscarVoxel(litVoxel.x, litVoxel.y, litVoxel.z);			
	if (type == 0){
		borrarVoxel(litVoxel.x, litVoxel.y, litVoxel.z);
		}
		
	litVoxel.x = x;
	litVoxel.y = y - 1;
	litVoxel.z = z;			
	type = buscarVoxel(litVoxel.x, litVoxel.y, litVoxel.z);
	if (type == 0){
		borrarVoxel(litVoxel.x, litVoxel.y, litVoxel.z);
	}
	
	litVoxel.x = x + 1;
	litVoxel.y = y;
	litVoxel.z = z;			
	type = buscarVoxel(litVoxel.x, litVoxel.y, litVoxel.z);
	if (type == 0){
		borrarVoxel(litVoxel.x, litVoxel.y, litVoxel.z);
	}
			
	litVoxel.x = x - 1;
	litVoxel.y = y;
	litVoxel.z = z;			
	type = buscarVoxel(litVoxel.x, litVoxel.y, litVoxel.z);
	if (type == 0){
		borrarVoxel(litVoxel.x, litVoxel.y, litVoxel.z);
	}
			
	litVoxel.x = x;
	litVoxel.y = y;
	litVoxel.z = z + 1;			
	type = buscarVoxel(litVoxel.x, litVoxel.y, litVoxel.z);
	if (type == 0){
		borrarVoxel(litVoxel.x, litVoxel.y, litVoxel.z);
	}
	
	litVoxel.x = x;
	litVoxel.y = y;
	litVoxel.z = z - 1;				
	type = buscarVoxel(litVoxel.x, litVoxel.y, litVoxel.z);
	if (type == 0){
		borrarVoxel(litVoxel.x, litVoxel.y, litVoxel.z);
	}		
}

void mantainRadiance(){
	
	lightNear = false;
	int x = toDeleteVoxel.x;
	int y = toDeleteVoxel.y;
	int z = toDeleteVoxel.z;
	
	Voxel litVoxel = {x, y, z};	
	int type = 0;
	
	litVoxel.x = x;
	litVoxel.y = y + 1;
	litVoxel.z = z;	
	type = buscarVoxel(litVoxel.x, litVoxel.y, litVoxel.z);			
	if (type == 7){
		lightNear = true;
		}
		
	litVoxel.x = x;
	litVoxel.y = y - 1;
	litVoxel.z = z;			
	type = buscarVoxel(litVoxel.x, litVoxel.y, litVoxel.z);
	if (type == 7){
		lightNear = true;
	}
	
	litVoxel.x = x + 1;
	litVoxel.y = y;
	litVoxel.z = z;			
	type = buscarVoxel(litVoxel.x, litVoxel.y, litVoxel.z);
	if (type == 7){
		lightNear = true;
	}
			
	litVoxel.x = x - 1;
	litVoxel.y = y;
	litVoxel.z = z;			
	type = buscarVoxel(litVoxel.x, litVoxel.y, litVoxel.z);
	if (type == 7){
		lightNear = true;
	}
			
	litVoxel.x = x;
	litVoxel.y = y;
	litVoxel.z = z + 1;			
	type = buscarVoxel(litVoxel.x, litVoxel.y, litVoxel.z);
	if (type == 7){
		lightNear = true;
	}
	
	litVoxel.x = x;
	litVoxel.y = y;
	litVoxel.z = z - 1;				
	type = buscarVoxel(litVoxel.x, litVoxel.y, litVoxel.z);
	if (type == 7){
		lightNear = true;
	}	
}
////////////////////////////BRUSH////////////////////////////////

void drawCube() {

	if (notEmpty){
		if (pointerType == 0 || pointerType == 1){
		agregarVoxel(pointerVoxel.x, pointerVoxel.y, pointerVoxel.z, pointerType, 2);			
		}
		else if (pointerType == 7){
			agregarVoxel(pointerVoxel.x, pointerVoxel.y, pointerVoxel.z, 7, 2);	
			fillRadiance();		
		}
		else {
		agregarVoxel(pointerVoxel.x, pointerVoxel.y, pointerVoxel.z, pointerType, 1);//(rand() % 16)+1); 
		}
	playSound("C:/voxelito/bip.wav");
	Sleep(250);
	}
}

void eraseCube() {
	if (eraseable){		
		int typee = buscarVoxel(toDeleteVoxel.x, toDeleteVoxel.y, toDeleteVoxel.z);	
		
		if (typee == 7){
			emptyRadiance();
			borrarVoxel(toDeleteVoxel.x, toDeleteVoxel.y, toDeleteVoxel.z);
		}
		else {
			mantainRadiance();
			
			if (lightNear){
				borrarVoxel(toDeleteVoxel.x, toDeleteVoxel.y, toDeleteVoxel.z);
				agregarVoxel(toDeleteVoxel.x, toDeleteVoxel.y, toDeleteVoxel.z, 0, 2);
			}
			else {
				borrarVoxel(toDeleteVoxel.x, toDeleteVoxel.y, toDeleteVoxel.z);				
			}
		}
		playSound("C:/voxelito/bop.wav"); 
		Sleep(250);		
	}

}

/////////////////////////////COLISION////////////////////////////

void detectarColisiones(){
		
	float offset = 0.15;
	float repelInt = 0.10;
	
	Vector cam = {originX,originY,originZ};
	//Vector camint = {(int)floorf(originX), (int)floorf(originY), (int)floorf(originZ)};
	
    // Definir y rotar el vector front
    Vector front = {0.0f, 0.0f, offset};
    float rotatedXf = front.x * cos(-radY) - front.z * sin(-radY);
    float rotatedZf = front.x * sin(-radY) + front.z * cos(-radY);
    front.x = originX + rotatedXf;
    front.y = originY;
    front.z = originZ + rotatedZf;

    // Definir y rotar el vector back
    Vector back = {0.0f, 0.0f, -offset};
    float rotatedXb = back.x * cos(radY) - back.z * sin(radY);
    float rotatedZb = back.x * sin(radY) + back.z * cos(radY);
    back.x = originX + rotatedXb;
    back.y = originY;
    back.z = originZ + rotatedZb;

    // Calcular los vectores left y right en función del ángulo radY
    Vector left = {-offset, 0.0f, 0.0f};
    float rotatedXl = left.x * cos(radY) - left.z * sin(radY);
    float rotatedZl = left.x * sin(radY) + left.z * cos(radY);
    left.x = originX + rotatedXl;
    left.y = originY;
    left.z = originZ + rotatedZl;

    Vector right = {offset, 0.0f, 0.0f};
    float rotatedXr = right.x * cos(-radY) - right.z * sin(-radY);
    float rotatedZr = right.x * sin(-radY) + right.z * cos(-radY);
    right.x = originX + rotatedXr;
    right.y = originY;
    right.z = originZ + rotatedZr;

    // Vector up y down no necesitan rotación en Y ya que sólo se mueven en el eje Y
    Vector up = {originX, originY + offset, originZ};
    Vector down = {originX, originY - offset*6, originZ};
	
    colisionF = buscarVoxel((int)floorf(front.x), (int)floorf(front.y), (int)floorf(front.z)) != 0;
    colisionB = buscarVoxel((int)floorf(back.x), (int)floorf(back.y), (int)floorf(back.z)) != 0;
    colisionL = buscarVoxel((int)floorf(left.x), (int)floorf(left.y), (int)floorf(left.z)) != 0;
    colisionR = buscarVoxel((int)floorf(right.x), (int)floorf(right.y), (int)floorf(right.z)) != 0;
    colisionU = buscarVoxel((int)floorf(up.x), (int)floorf(up.y), (int)floorf(up.z)) != 0;
    colisionD = buscarVoxel((int)floorf(down.x), (int)floorf(down.y), (int)floorf(down.z)) != 0;
 
    //printf("origin: %.2f, %.2f, %.2f\n", originX, originY, originZ);          /////PRINT ORIGIN
}

///////////////////////CUATERNIONES//////////////////////////////

//QUAT FROM VECTOR AND AXIS
Quaternion crearCuat(float angle, float ux, float uy, float uz) {
    Quaternion q;
    float halfAngle = angle * 0.5f;
    float sinHalfAngle = sin(halfAngle);

    q.w = cos(halfAngle);
    q.x = ux * sinHalfAngle;
    q.y = uy * sinHalfAngle;
    q.z = uz * sinHalfAngle;

    return q;
}

//QUAT MULTIPLIER
Quaternion multCuats(Quaternion q1, Quaternion q2) {
    Quaternion result;

    result.w = q1.w * q2.w - q1.x * q2.x - q1.y * q2.y - q1.z * q2.z;
    result.x = q1.w * q2.x + q1.x * q2.w + q1.y * q2.z - q1.z * q2.y;
    result.y = q1.w * q2.y - q1.x * q2.z + q1.y * q2.w + q1.z * q2.x;
    result.z = q1.w * q2.z + q1.x * q2.y - q1.y * q2.x + q1.z * q2.w;

    return result;
}

//ROTATE VECTOR WITH QUAT
void rotarVectorConCuaternion(float* vecX, float* vecY, float* vecZ, Quaternion q) {
    Quaternion qConjugado = {q.w, -q.x, -q.y, -q.z}; // Conjugado del cuaternión
    Quaternion v = {0.0f, *vecX, *vecY, *vecZ}; // Vector como cuaternión

    // Rotación: q * v * q_conjugado
    Quaternion result = multCuats(multCuats(q, v), qConjugado);

    *vecX = result.x;
    *vecY = result.y;
    *vecZ = result.z;
}

////////////////////////// CAM //////////////////////////////////

void rotarCam(){
    // Ajustar los ángulos de rotación con E/Q para el eje Y y PD/PU para el eje X
    angleY += (GetAsyncKeyState(0x45) ? rotspeed : 0.0f); // E: Rotación en Y positiva
    angleY -= (GetAsyncKeyState(0x51) ? rotspeed : 0.0f); // Q: Rotación en Y negativa
    angleX += (GetAsyncKeyState(0x22) ? rotspeed : 0.0f); // PD: Rotación en X positiva
    angleX -= (GetAsyncKeyState(0x21) ? rotspeed : 0.0f); // PU: Rotación en X negativa

    // Convertir ángulos a radianes
    radY = angleY * (M_PI / 180.0f);
    radX = angleX * (M_PI / 180.0f);

    // Crear cuaterniones de rotación
    Quaternion qY = crearCuat(radY, 0.0f, 1.0f, 0.0f); // Rotación alrededor del eje Y
    Quaternion qX = crearCuat(radX, 1.0f, 0.0f, 0.0f); // Rotación alrededor del eje X local
	
	// Combinar las rotaciones: aplicar primero la rotación en Y, luego en X sobre el eje local de la cámara
    Quaternion qFinal = multCuats(qY, qX);

    // Vector inicial hacia adelante en el eje Z
    relX = 0.0f;
    relY = 0.0f;
    relZ = 1.0f;

	// Aplicar la rotación combinada al vector de dirección
    rotarVectorConCuaternion(&relX, &relY, &relZ, qFinal);

    // Aplicar la rotación en Y (eje global) al vector de dirección
    //rotarVectorConCuaternion(&relX, &relY, &relZ, qY);

    // Aplicar la rotación en X sobre el sistema de ejes ya rotado
    //rotarVectorConCuaternion(&relX, &relY, &relZ, qX);	
}

void tripleD(Vector origin, Vector end) {
	isVox = false;

    // Distancia del vector en cada eje
	
    float dx = (end.x - origin.x)*maxZ;
    float dy = (end.y - origin.y)*maxZ;
    float dz = (end.z - origin.z)*maxZ;
	
	//float dx = (comp.x - origin.x)*maxZ;
    //float dy = (comp.y - origin.y)*maxZ;
    //float dz = (comp.z - origin.z)*maxZ;

    // Número máximo de pasos para llegar al final
	float steps = fmaxf(fabsf(dx), fmaxf(fabsf(dy), fabsf(dz))) * stepsMult; // ((4 + rand() % 4) * stepsMult)
	
    float halfSteps = steps / 4;  // Punto para cambiar a menor detalle // 1° LOD //////////////////////////

	//STEP.XYZ
	float stepX = dx / steps; 
	float stepY = dy / steps;
	float stepZ = dz / steps;

    // Inicializar la posición en el primer vóxel(float)
    float x = origin.x;
    float y = origin.y;
    float z = origin.z;

	truGlass = false;

	bool lodChanged = false;  // Variable de control para evitar múltiples cambios de LOD

    for (int i = 0; i <= steps; i++) {
		
		// Calcular la posición relativa al centro de la entidad
		// Calcula el desplazamiento para centrar el mapa
		int centerOffset = ENTITY_MAP_SIZE / 2;

		int relX = (int)((x - ent.x) * ENTITY_MAP_SIZE) + centerOffset;
		int relY = (int)((y - ent.y) * ENTITY_MAP_SIZE) + centerOffset;
		int relZ = (int)((z - ent.z) * ENTITY_MAP_SIZE) + centerOffset;

		// Verifica si el punto está dentro de los límites del mapa
		if (relX >= 0 && relX < ENTITY_MAP_SIZE && relY >= 0 && relY < ENTITY_MAP_SIZE && relZ >= 0 && relZ < ENTITY_MAP_SIZE) {
			if (ent.map[relY][relX] == 1) {  // Comparar con el mapa en 2D
			isEntity = true;
			break;
			}
		}
	
        //Vóxel actual (redondeo a entero)
		Voxel currentVoxel = { (int)floorf(x), (int)floorf(y), (int)floorf(z) };
		
		//if (((x >= ent.x - 0.1f)&&(x <= ent.x + 0.1f)) && ((y >= ent.y - 0.1f)&&(y <= ent.y + 0.1f)) && ((z >= ent.z - 0.1f)&&(z <= ent.z + 0.1f))){
		//	isEntity = true;
		//	break;
		//}
		
		//type voxel
        type = buscarVoxel(currentVoxel.x, currentVoxel.y, currentVoxel.z);  // Busca el tipo del vóxel
		
        lit = buscarVoxelLit(currentVoxel.x, currentVoxel.y, currentVoxel.z);  // Busca el tipo del vóxel

		if (i >= fogStart){
		fogValue++;			
		}
		else{
			fogValue = 1.0;
		}

		if (lit == 2){
			fogValue = 1.00;
		}

		if (type == 16){
			isGlass = true;
			
			//GLASSFRAME
			// Obtener la parte decimal de las coordenadas
			float localX = x - currentVoxel.x;
			float localY = y - currentVoxel.y;
			float localZ = z - currentVoxel.z;
			
			// Convertir la parte decimal a índices de la matriz 8x8
			int mapX, mapY;
			outline = 1.0; // Valor predeterminado sin outline

			// Comprobar en cada cara
			if ((localX < 0.125f || localX > 0.875f)) {
				mapX = (int)(localZ * 8) % 8;
				mapY = (int)(localY * 8) % 8;
				if (frameMap[mapY][mapX] == 0) {
					outline = borderIntensity;
				}
			}
			if ((localY < 0.125f || localY > 0.875f)) {
				mapX = (int)(localX * 8) % 8;
				mapY = (int)(localZ * 8) % 8;
				if (frameMap[mapY][mapX] == 0) {
					outline = borderIntensity;
				}
			}
			if ((localZ < 0.125f || localZ > 0.875f)) {
				mapX = (int)(localX * 8) % 8;
				mapY = (int)(localY * 8) % 8;
				if (frameMap[mapY][mapX] == 0) {
					outline = borderIntensity;
				}
			}

			//END OF GLASSFRAME
						
			if (isCenter){ //decl of pointer
				pointerVoxel.x = previousVoxel.x;
				pointerVoxel.y = previousVoxel.y;
				pointerVoxel.z = previousVoxel.z;	
				notEmpty = true;
				toDeleteVoxel.x = currentVoxel.x;
				toDeleteVoxel.y = currentVoxel.y;
				toDeleteVoxel.z = currentVoxel.z;
				eraseable = true;
				truGlass = true;
				break; 
			}

        }

		
        if (type != 0 && type != 16) {

		//{0, 0, 0}, 	   // Color type 0 (negro) VOID
		//{255, 0, 0},     // Color type 1 (rojo) LAVA
		//{0, 255, 0},     // Color type 2 (verde) WAVE
		//{128, 64, 0},    // Color type 3 (marron) BRICK
		//{0, 255, 255},   // Color type 4 (cian) WATER
		//{255, 0, 255},   // Color type 5 (marron) BARS
		//{255, 255, 0},   // Color type 6 (amarillo) WAVE/DIAGONAL?
		//{255, 255, 255}, // Color type 7 (blanco)
		//{128, 128, 128}, // Color type 8 (gris) CHECKER
		//{128, 0, 0},     // Color type 9 (bordo) CELLS
		//{0, 128, 0},     // Color type 10 (verde oscuro) TISSUE
		//{0, 0, 128},     // Color type 11 (azul oscuro)
		//{128, 128, 0},   // Color type 12 (oliva) PLANT
		//{128, 0, 128},   // Color type 13 (púrpura)
		//{0, 128, 128},   // Color type 14 (verde azulado)
		//{192, 192, 192}, // Color type 15 (gris claro)
		//{0, 255, 255},   // Color type 16 (vidrio cian)
		//{0, 0, 255}	   // Color type 18 (azul exmarron)


			if (type == 1){
				if (animVoronoiPattern3D(x, y, z, 0.3)) {  // Puedes ajustar `cellSize` según el tamaño deseado del patrón
					outline = borderIntensity;
				} else {
				outline = 1.0;
				}				
				
			}			

			if (type == 2){
				if (cloudPattern3D(x, y, z, 0.5f) == 1) {  // Puedes ajustar `cellSize` según el tamaño deseado del patrón
					outline = borderIntensity;
				} else {
				outline = 1.0;
				}				
				
			}	

			if (type == 3){
				// Obtener la parte decimal de las coordenadas
				float localX = x - currentVoxel.x;
				float localY = y - currentVoxel.y;
				float localZ = z - currentVoxel.z;

				// Convertir la parte decimal a índices de la matriz 8x8
				int mapX, mapY;
				outline = 1.0; // Valor predeterminado sin outline

				// Comprobar en cada cara
				if ((localX < 0.125f || localX > 0.875f)) {
					mapX = (int)(localZ * 8) % 8;
					mapY = (int)(localY * 8) % 8;
					if (brickMap[mapY][mapX] == 1) {
						outline = borderIntensity;
					}
				}
				if ((localY < 0.125f || localY > 0.875f)) {
					mapX = (int)(localX * 8) % 8;
					mapY = (int)(localZ * 8) % 8;
					if (brickMap[mapY][mapX] == 1) {
						outline = borderIntensity;
					}
				}
				if ((localZ < 0.125f || localZ > 0.875f)) {
					mapX = (int)(localX * 8) % 8;
					mapY = (int)(localY * 8) % 8;
					if (brickMap[mapY][mapX] == 1) {
						outline = borderIntensity;
					}
			}
			}

			if (type == 4){
				if (animVoronoiPattern3D(x, y, z, 0.1)) {  // Puedes ajustar `cellSize` según el tamaño deseado del patrón
					outline = borderIntensity;
				} else {
				outline = 1.0;
				}				
				
			}	

			if (type == 5){
				// Obtener la parte decimal de las coordenadas
				float localX = x - currentVoxel.x;
				float localY = y - currentVoxel.y;
				float localZ = z - currentVoxel.z;

				// Convertir la parte decimal a índices de la matriz 8x8
				int mapX, mapY;
				outline = 1.0; // Valor predeterminado sin outline

				// Comprobar en cada cara
				if ((localX < 0.125f || localX > 0.875f)) {
					mapX = (int)(localZ * 8) % 8;
					mapY = (int)(localY * 8) % 8;
					if (woodMap[mapY][mapX] == 1) {
						outline = borderIntensity;
					}
				}
				if ((localY < 0.125f || localY > 0.875f)) {
					mapX = (int)(localX * 8) % 8;
					mapY = (int)(localZ * 8) % 8;
					if (woodMap[mapY][mapX] == 1) {
						outline = borderIntensity;
					}
				}
				if ((localZ < 0.125f || localZ > 0.875f)) {
					mapX = (int)(localX * 8) % 8;
					mapY = (int)(localY * 8) % 8;
					if (woodMap[mapY][mapX] == 1) {
						outline = borderIntensity;
					}
			}
			}
					
			if (type == 6){
				if (diagonalStripePattern3D(x, y, z, 0.1f) == 1) {  // Puedes ajustar `cellSize` según el tamaño deseado del patrón
					outline = borderIntensity;
				} else {
				outline = 1.0;
				}				
				
			}	

			if (type == 7){
				// Obtener la parte decimal de las coordenadas
				float localX = x - currentVoxel.x;
				float localY = y - currentVoxel.y;
				float localZ = z - currentVoxel.z;

				// Convertir la parte decimal a índices de la matriz 8x8
				int mapX, mapY;
				outline = 1.0; // Valor predeterminado sin outline

				// Comprobar en cada cara
				if ((localX < 0.125f || localX > 0.875f)) {
					mapX = (int)(localZ * 8) % 8;
					mapY = (int)(localY * 8) % 8;
					if (frameMap[mapY][mapX] == 1) {
						outline = borderIntensity;
					}
				}
				if ((localY < 0.125f || localY > 0.875f)) {
					mapX = (int)(localX * 8) % 8;
					mapY = (int)(localZ * 8) % 8;
					if (frameMap[mapY][mapX] == 1) {
						outline = borderIntensity;
					}
				}
				if ((localZ < 0.125f || localZ > 0.875f)) {
					mapX = (int)(localX * 8) % 8;
					mapY = (int)(localY * 8) % 8;
					if (frameMap[mapY][mapX] == 1) {
						outline = borderIntensity;
					}
			}
			}

			if (type == 8){
				if (gridPattern3D(x, y, z, 1.0)) {  // Puedes ajustar `cellSize` según el tamaño deseado del patrón
					outline = borderIntensity;
				} else {
				outline = 1.0;
				}				
				
			}
	
			if (type == 9){
				// Obtener la parte decimal de las coordenadas
				float localX = x - currentVoxel.x;
				float localY = y - currentVoxel.y;
				float localZ = z - currentVoxel.z;

				// Convertir la parte decimal a índices de la matriz 8x8
				int mapX, mapY;
				outline = 1.0; // Valor predeterminado sin outline

				// Comprobar en cada cara
				if ((localX < 0.125f || localX > 0.875f)) {
					mapX = (int)(localZ * 8) % 8;
					mapY = (int)(localY * 8) % 8;
					if (metalMap[mapY][mapX] == 1) {
						outline = borderIntensity;
					}
				}
				if ((localY < 0.125f || localY > 0.875f)) {
					mapX = (int)(localX * 8) % 8;
					mapY = (int)(localZ * 8) % 8;
					if (metalMap[mapY][mapX] == 1) {
						outline = borderIntensity;
					}
				}
				if ((localZ < 0.125f || localZ > 0.875f)) {
					mapX = (int)(localX * 8) % 8;
					mapY = (int)(localY * 8) % 8;
					if (metalMap[mapY][mapX] == 1) {
						outline = borderIntensity;
					}
			}
			}

			if (type == 10){
				if (meshPattern3D(x, y, z, 23.0f, 0.8f, 0.45f)) {  // Puedes ajustar `cellSize` según el tamaño deseado del patrón
					outline = borderIntensity;
				} else {
				outline = 1.0;
				}				
				
			}		

			if (type == 12){
				// Obtener la parte decimal de las coordenadas
				float localX = x - currentVoxel.x;
				float localY = y - currentVoxel.y;
				float localZ = z - currentVoxel.z;

				// Convertir la parte decimal a índices de la matriz 8x8
				int mapX, mapY;
				outline = 1.0; // Valor predeterminado sin outline

				// Comprobar en cada cara
				if ((localX < 0.125f || localX > 0.875f)) {
					mapX = (int)(localZ * 8) % 8;
					mapY = (int)(localY * 8) % 8;
					if (sandMap[mapY][mapX] == 1) {
						outline = borderIntensity;
					}
				}
				if ((localY < 0.125f || localY > 0.875f)) {
					mapX = (int)(localX * 8) % 8;
					mapY = (int)(localZ * 8) % 8;
					if (sandMap[mapY][mapX] == 1) {
						outline = borderIntensity;
					}
				}
				if ((localZ < 0.125f || localZ > 0.875f)) {
					mapX = (int)(localX * 8) % 8;
					mapY = (int)(localY * 8) % 8;
					if (sandMap[mapY][mapX] == 1) {
						outline = borderIntensity;
					}
			}
			}

			//border obsolete/deprecated
			//if (((x - currentVoxel.x) > preMargin && (x - currentVoxel.x) < borderTolerance) ||  // Borde en X
			//((y - currentVoxel.y) > preMargin && (y - currentVoxel.y) < borderTolerance) ||  // Borde en Y
			//((z - currentVoxel.z) > preMargin && (z - currentVoxel.z) < borderTolerance))   // Borde en Z
			//{  
				
			//	outline = borderIntensity;
			//}
			//
			//else { //borde
			//	outline = 1.0;
			//}
			
			if (isCenter && truGlass == false){ //decl of pointer
				pointerVoxel.x = previousVoxel.x;
				pointerVoxel.y = previousVoxel.y;
				pointerVoxel.z = previousVoxel.z;	
				notEmpty = true;
				toDeleteVoxel.x = currentVoxel.x;
				toDeleteVoxel.y = currentVoxel.y;
				toDeleteVoxel.z = currentVoxel.z;
				eraseable = true;
			}
            break;  // Detener si se encuentra un vóxel en la lista
        }

		outline = 1.0;
		

		if (isCenter){
		previousVoxel = currentVoxel;	
		}

        // Cambiar a menor detalle si se ha recorrido la mitad de los pasos sin encontrar un vóxel
        if (i >= halfSteps && !lodChanged) {
			
            steps = (steps - i) / 2; // Reducir los pasos restantes a la mitad //2° LOD /////////////////////
			
            stepX *= 8;//2;  // Duplicar el tamaño del paso para cubrir más distancia
            stepY *= 8;//2;
            stepZ *= 8;//2;
            lodChanged = true;  // Asegura que sólo cambie el LOD una vez
        }

        // Avanzar al siguiente punto en el espacio
        x += stepX;
        y += stepY;
        z += stepZ;
		
		if (isCenter && type != 16){
		notEmpty = false;
		eraseable = false;
		}
    }

}

void planeBuffer() {
		
	// Crear cuaternión de rotación combinado una vez por fotograma
    Quaternion qY = crearCuat(radY, 0.0f, 1.0f, 0.0f);
    Quaternion qX = crearCuat(radX, 1.0f, 0.0f, 0.0f);
    Quaternion qFinal = multCuats(qY, qX); // Rotación combinada Y -> X
	
    Vector originPoint = {originX, originY, originZ};
	
    for (int x = 0; x < WIDTH; x++) { // Recorre cada fila
        for (int y = 0; y < HEIGHT; y++) { // Recorre cada columna
            //type = 0;
            Vector endPoint = {
                (x - (WIDTH / 2.0)) * aperture,
                (y - (HEIGHT / 2.0)) * aperture,
                dof
            };

			// Rotar el punto final utilizando el cuaternión combinado
            rotarVectorConCuaternion(&endPoint.x, &endPoint.y, &endPoint.z, qFinal);

            endPoint.x += originX;
            endPoint.y += originY;
            endPoint.z += originZ;

			if ((x == (WIDTH / 2.0)) && (y == (HEIGHT / 2.00))){
				isCenter = true;
			}
			
            tripleD(originPoint, endPoint);
			
			//outline = 1;

			if (lit == 2){                /////////////emissive material
				fogValue = 1.00;
			}

            Color color = colorDictionary[type];
            pixels[y][x * 4 + 0] = (color.b * outline)/(fogValue);
            pixels[y][x * 4 + 1] = (color.g * outline)/(fogValue);
            pixels[y][x * 4 + 2] = (color.r * outline)/(fogValue);
            pixels[y][x * 4 + 3] = 255; // Alpha
		
			if (isGlass){			
			pixels[y][x * 4 + 0] = 20 + ((color.b * outline)*0.5f)/(fogValue);//* 1/(fogValue+fogInt);
			pixels[y][x * 4 + 1] = 10 + ((color.g * outline)*0.5f)/(fogValue);//* 1/(fogValue+fogInt);
			pixels[y][x * 4 + 2] = 10 + ((color.r * outline)*0.5f)/(fogValue);//* 1/(fogValue+fogInt);
			pixels[y][x * 4 + 3] = 255; // Alpha	

				if (isEntity){
				
				pixels[y][x * 4 + 0] = 20 + (ent.col.b*0.5f)/fogValue;
				pixels[y][x * 4 + 1] = 10 + (ent.col.g*0.5f)/fogValue;
				pixels[y][x * 4 + 2] = 10 + (ent.col.r*0.5f)/fogValue;
				pixels[y][x * 4 + 3] = 255; // Alpha			
				isEntity = false;
				}

			isGlass = false;
			}	

			if (isEntity){
				
            pixels[y][x * 4 + 0] = ent.col.b/fogValue;
            pixels[y][x * 4 + 1] = ent.col.g/fogValue;
            pixels[y][x * 4 + 2] = ent.col.r/fogValue;
            pixels[y][x * 4 + 3] = 255; // Alpha			
			isEntity = false;
			}

			isGlass = false;
			
			if (isCenter){ //crosshair
            Color color = colorDictionary[pointerType];			
            pixels[y][x * 4 + 0] = color.b; 
            pixels[y][x * 4 + 1] = color.g;
            pixels[y][x * 4 + 2] = color.r;
            pixels[y][x * 4 + 3] = 255;			
			}
			
			isCenter = false;			

        }
    }
}

void cam() {
    // Activar o desactivar la cámara con ESC
    isActive *= (GetAsyncKeyState(0x1B) ? 0 : 1);
	
	// Capturar el movimiento del mouse
    POINT currentMousePos;
    GetCursorPos(&currentMousePos);
	
	if (isMouseInitialized) {	
	
		infiniteWrap();
	
		//int deltaX = freeX - previousFreeX;
        //int deltaY = freeY - previousFreeY;
        // Ajustar ángulos de rotación en función del desplazamiento del mouse
        //angleY += deltaX * 0.5f; // Multiplica por un factor para ajustar la sensibilidad
        //angleX += deltaY * 0.5f; // Multiplica por un factor para ajustar la sensibilidad
		
    } else {
        isMouseInitialized = true;
    }
	
	previousFreeX = freeX;
	previousFreeY = freeY;
	//lastMousePos = currentMousePos; // Actualizar la última posición del mouse
	while (ShowCursor(FALSE) >= 0);  // Ocultar el cursor
	
	rotarCam();

	detectarColisiones();

    // Movimiento basado en la dirección de la cámara
    float moveX = 0.0f, moveY = 0.0f, moveZ = 0.0f;

	if (!colisionF && GetAsyncKeyState(0x57)) {
		moveZ += speed;
		}; // W: Mover hacia adelante
	if (!colisionB && GetAsyncKeyState(0x53)) {
		moveZ -= speed;
		}; // S: Mover hacia atrás
	if (!colisionL && GetAsyncKeyState(0x41)) {
		moveX -= speed;
		}; // A: Mover a la izquierda
	if (!colisionR && GetAsyncKeyState(0x44)) {
		moveX += speed;
		}; // D: Mover a la derecha
	if (!colisionD && GetAsyncKeyState(VK_SHIFT)) {
		moveY -= speed;
		}; // Shift: Bajar
	if (!colisionU && colisionD && GetAsyncKeyState(VK_SPACE)) {
		moveY += jumpspeed;
		}; // Espacio: Subir
	if (!colisionD && gravityActivated == true){
		gravity();
		};
	if (!colisionU && !colisionD && GetAsyncKeyState(VK_SPACE) && gravityActivated == false){
		moveY += speed;
		};
	
	
    // Movimiento de la cámara en las direcciones según las teclas presionadas
    //moveZ += (GetAsyncKeyState(0x57) ? speed : 0.0f); // W: Mover hacia adelante
    //moveZ -= (GetAsyncKeyState(0x53) ? speed : 0.0f); // S: Mover hacia atrás
    //moveX -= (GetAsyncKeyState(0x41) ? speed : 0.0f); // A: Mover a la izquierda
    //moveX += (GetAsyncKeyState(0x44) ? speed : 0.0f); // D: Mover a la derecha
    //moveY -= (GetAsyncKeyState(VK_SHIFT) ? speed : 0.0f); // Shift: Bajar
    //moveY += (GetAsyncKeyState(VK_SPACE) ? speed : 0.0f); // Espacio: Subir

    // Aplicar el movimiento en función de la dirección de la cámara
    originX += relX * moveZ + cos(radY) * moveX;
    //originY += relY * moveZ + moveY;
	originY += moveY;
    originZ += relZ * moveZ - sin(radY) * moveX;

    // Calcular el punto final de la cámara basado en la dirección de rel
    endX = originX + relX;
    endY = originY + relY;
    endZ = originZ + relZ;
	
	Vector camera = {originX,originY,originZ};
	
    if (consola) {
        // Aquí puedes imprimir los valores para verificar los cambios
    }

    planeBuffer();
}

/////////////////////////RENDER///////////////////////////////////

void renderizarBuffer(HDC hdc) {
    // Crear un bitmap compatible
    HBITMAP hBitmap = CreateDIBSection(hdc, &bi, DIB_RGB_COLORS, NULL, NULL, 0);
    if (!hBitmap) {
        MessageBox(NULL, "Error al crear DIB section", "Error", MB_OK);
        return; // Si hay un error, muestra un mensaje y sale
    }

    // Copiar los colores del buffer al bitmap
    HDC hMemDC = CreateCompatibleDC(hdc); // Crea un contexto de dispositivo compatible
    SelectObject(hMemDC, hBitmap); // Selecciona el bitmap en el contexto de memoria
    
    // Actualizar el bitmap con los colores del buffer
    SetDIBits(hMemDC, hBitmap, 0, HEIGHT, pixels, &bi, DIB_RGB_COLORS);

    // Dibujar el bitmap escalado en la ventana
    StretchBlt(hdc, 0, 0, WIDTH * SCALE, HEIGHT * SCALE, hMemDC, 0, 0, WIDTH, HEIGHT, SRCCOPY);

    // Limpiar
    DeleteDC(hMemDC); // Eliminar el contexto de memoria
    DeleteObject(hBitmap); // Eliminar el bitmap
}

////////////////////////CONSOLA///////////////////////////////////

void crearConsola() {
    AllocConsole(); // Aloca una nueva consola
    FILE* f;
    freopen("CONOUT$", "w", stdout); // Redirige la salida estándar a la nueva consola
    printf("DATA:\n"); // Mensaje inicial en la consola
}

//MAIN
int main() {
	
	srand(seed);
	
    cargarVoxeles("voxeles.bin");	
	//inicializarVoxeles();
	//dibujarEsfera();
	
	//initializeEntityMap(&ent);	/////////////////////////entity
	
    // Inicializar la semilla aleatoria
    srand(time(0)); // Semilla para generar números aleatorios

    // Inicializar el BITMAPINFO
    bi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER); // Tamaño de la cabecera
    bi.bmiHeader.biWidth = WIDTH; // Ancho del bitmap
    bi.bmiHeader.biHeight = HEIGHT; // Altura del bitmap (negativo para que esté en la parte superior)
    bi.bmiHeader.biPlanes = 1; // Número de planos
    bi.bmiHeader.biBitCount = 32; // 32 bits por píxel
    bi.bmiHeader.biCompression = BI_RGB; // Sin compresión
    bi.bmiHeader.biSizeImage = 0; // Tamaño de la imagen (0 para bitmap sin compresión)
    bi.bmiHeader.biXPelsPerMeter = 0; // Resolución horizontal
    bi.bmiHeader.biYPelsPerMeter = 0; // Resolución vertical
    bi.bmiHeader.biClrUsed = 0; // Colores utilizados (0 para todos)
    bi.bmiHeader.biClrImportant = 0; // Colores importantes (0 para todos)

    // Crear consola para el número de frames
    crearConsola();

    // Registrar la clase de ventana
    const char* CLASS_NAME = "voxelito"; // Nombre de la clase de la ventana
    WNDCLASS wc = { 0 }; // Inicializa la estructura WNDCLASS
    wc.lpfnWndProc = WindowProc; // Asigna el procedimiento de ventana
    wc.hInstance = GetModuleHandle(NULL); // Obtiene el manejador de la instancia actual
    wc.lpszClassName = CLASS_NAME; // Asigna el nombre de la clase

    RegisterClass(&wc); // Registra la clase de la ventana

    // Crear la ventana
    HWND hwnd = CreateWindowEx(
        0,
        CLASS_NAME, // Clase de ventana
        "voxelito by tezzerakto", // Título de la ventana
        WS_OVERLAPPEDWINDOW, // Estilo de ventana
        CW_USEDEFAULT, CW_USEDEFAULT, (WIDTH * SCALE)+16, (HEIGHT * SCALE)+16, // Posición y tamaño de la ventana
        NULL, NULL, GetModuleHandle(NULL), NULL // Parámetros adicionales
    );

    if (hwnd == NULL) {
        return 0; // Si no se pudo crear la ventana, sale
    }

    ShowWindow(hwnd, SW_SHOW); // Muestra la ventana

    // Crear un contexto de dispositivo (DC) para la ventana
    HDC hdc = GetDC(hwnd); // Obtiene el contexto de dispositivo para la ventana

    // Bucle de mensajes y actualización de la ventana
    MSG msg = { 0 }; // Inicializa la estructura MSG
    //int frameCount = 0; // Contador de frames
	
	centrarVentana(hwnd);


    while (isActive) { // Bucle infinito
        // Procesar mensajes de Windows
        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) { // Busca mensajes en la cola
            if (msg.message == WM_QUIT) { // Si se recibe el mensaje de cerrar
                ReleaseDC(hwnd, hdc); // Libera el contexto de dispositivo
                return 0; // Sale del programa
            }
            TranslateMessage(&msg); // Traduce el mensaje
            DispatchMessage(&msg); // Despacha el mensaje
        }
		
		captureWheel(); //captura rueda y define typo		

		drawVox = 0;
					
		cam(); ///////////////////////

		followTarget(); //////////////
		
		drawVox += (GetAsyncKeyState(0x52) ? 1 : 0); //drawvox
		if (drawVox == 1){
			drawCube();	
			drawVox = 0;
		}
		
		deleteVox += (GetAsyncKeyState(0x54) ? 1 : 0);
		if (deleteVox == 1){
			eraseCube();
			deleteVox = 0;
		}
        // Renderizar el buffer en la ventana
        renderizarBuffer(hdc);

        // Contar el número de frames
        frameCount++;
    }
	printf("Enter para cerrar...");
    getchar(); // Pausa hasta que el usuario presione Enter
	liberarVoxeles();
    return 0; // Fin del programa
}
