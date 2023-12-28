#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define SCREEN_WIDTH 1280
#define SCREEN_HEIGHT 720

#define CHUNK_WIDTH 64
#define CHUNK_HEIGHT 64
#define CHUNK_SIZE CHUNK_WIDTH*CHUNK_HEIGHT

#define TERMINAL_WIDTH 10
#define TERMINAL_HEIGHT 10

#define VIEW_RANGE_X 7
#define VIEW_RANGE_Y 7

/* Constants */
typedef enum CellType { Empty, } CellType;
char CellChars[] = { '.' };
char * Messages[ ] = { "", "Invalid Command!\n", "Here be dragons.\n", "You move north\n", "You move south\n", "You move east\n", "You move west\n", "Something blocks your way\n" };

/* Utils */
char getCellChar (CellType type) { return CellChars[(int) type]; }

/* Structs */
typedef struct Vector2 { int x; int y; } Vector2;
typedef struct Cell { Vector2 pos; CellType type; } Cell;
typedef struct Chunk { Cell ** cells; Vector2 pos; } Chunk;
typedef struct Soul { char * name; int age; } Soul;
typedef struct Gnome { Soul * soul; Vector2 pos; char c; } Gnome;

/* Globals */
char ** terminal;
Chunk * region;
Gnome ** gnomes;
Gnome * player;
int gnome_pop;
int tick = 0;
int is_running = 1;
int message = 0;
Vector2 camera_pos = { 0, 0 };

Vector2 newVector2 (int x, int y) {
	Vector2 result = { x, y };
	return result;
}

Cell * newCell (int x, int y) {
	Cell * result = (Cell *) malloc(sizeof(Cell));
	if (!result) { return NULL; }
	result->pos = newVector2(x, y);
	result->type = 0;
	return result;
}

void freeCell (Cell * target) {
	if (!target) { return; }
	free(target);
	return;
}

int drawCell (Cell * target) {
	if (!target) { return 1; }
	char c = getCellChar(target->type);
	printf(" %c ", c);
	return 0;
}

Chunk * newChunk (int x, int y) {
	Chunk * result = (Chunk *) malloc(sizeof(Chunk));
	if (!result) { return NULL; }
	result->cells = (Cell **) malloc(sizeof(Cell *) * CHUNK_SIZE);
	if (!result->cells) { free(result); return NULL; }
	for (int i = 0; i < CHUNK_SIZE; i++) {
		result->cells[i] = newCell(i % CHUNK_WIDTH, i / CHUNK_WIDTH);
		if (!result->cells[i]) {
			for (int j = i - 1; j >= 0; j--) { freeCell(result->cells[i]); }
			free(result->cells); free(result); return NULL;
		}
	}
	return result;
}

void freeChunk (Chunk * target) {
	if (!target) { return; }
	if (!target->cells) { free(target); return; }
	for (int i = 0; i < CHUNK_SIZE; i++) {
		if (!target->cells[i]) { continue; }
		freeCell(target->cells[i]);
	}
	free(target->cells);
	free(target);
	return;
}

Soul * newSoul () {
	Soul * result = (Soul *) malloc(sizeof(Soul));
	if (!result) { return NULL; }
	result->age = 0;
	result->name = (char *) malloc(sizeof(char) * 10);
	if (!result->name) { free(result);  return NULL; }
	for (int i = 0; i < 9; i++) { result->name[i] = "undefined"[i]; }
	result->name[9] = '\0';
	return result;
}

void freeSoul (Soul * target) {
	if (!target) { return; }
	if (!target->name) { free(target); return; }
	free(target->name);
	free(target);
	return;
}

Gnome * newGnome (int x, int y) {
	Gnome * result = (Gnome *) malloc(sizeof(Gnome));
	if (!result) { return NULL; }
	result->pos = newVector2(x, y);
	result->soul = newSoul();
	result->c = 'G';
	if (!result->soul) { free(result); return NULL; }
	return result;
}

void freeGnome(Gnome * target) {
	if (!target) { return; }
	if (!target->soul) { free(target); return; }
	freeSoul(target->soul);
	free(target);
	return;
}

Gnome * spawnGnome (int x, int y) {
	Gnome * result = newGnome(x, y);
	if (!result) { return NULL; }
	gnomes[gnome_pop] = result;
	gnome_pop++;
	gnomes = (Gnome **) realloc(gnomes, sizeof(Gnome *) * (gnome_pop + 1));
	if (!gnomes) { return NULL; }
	return result;
}

int initGlobals () {
	region = newChunk(0, 0);
	terminal = (char **) malloc(sizeof(char *) * TERMINAL_HEIGHT);
	if (!terminal) { return 1; }
	for (int i = 0; i < TERMINAL_HEIGHT; i++) {
		terminal[i] = (char *) malloc(sizeof(char) * TERMINAL_WIDTH);
		if (!terminal[i]) {
			for (int j = i - 1; j <= 0; j--) { free(terminal[j]); }
			free(terminal);
			return 2;
		}
		for (int j = 0; j < TERMINAL_WIDTH; j++) { terminal[i][j] = ' '; }
	}
	gnome_pop = 0;
	gnomes = (Gnome **) malloc(sizeof(Gnome *));
	if (!gnomes) { return 3; }
	return 0;
}

void freeGlobals () {
	if (region) {
		freeChunk(region);
	}
	if (terminal) {
		for (int i = 0; i < TERMINAL_HEIGHT; i++) { if (terminal[i]) { free(terminal[i]); } }
		free(terminal);
	}
	if (gnomes) {
		for (int i = 0; i < gnome_pop; i++) { if (gnomes[i]) { freeGnome(gnomes[i]); } }
		free(gnomes);
	}
}

void setCameraPos (int x, int y) { camera_pos = newVector2(x, y); }
void drawTerminal () {
	for (int i = 0; i < TERMINAL_HEIGHT; i++) {
		if (!terminal[i]) { printf("[ERROR]\n"); continue; }
		for (int j = 0; j < TERMINAL_WIDTH; j++) {
			printf("%c", terminal[i][j]);
		}
		printf("\n");
	}
	return;
}
int setTerminal (char c, int x, int y) {
	if (x < 0 || x >= TERMINAL_WIDTH || y < 0 || y >= TERMINAL_HEIGHT) { return 1; }
	if (!terminal || !terminal[y]) { return 2; }
	terminal[y][x] = c;
	return 0;
}
int drawChunk (Chunk * target) {
	if (!target) { return 1; }
	for (int i = camera_pos.y; i-camera_pos.y < VIEW_RANGE_Y; i++) {
		for (int j = camera_pos.x; j-camera_pos.x < VIEW_RANGE_X; j++) {
			int x = j;
			int y = i * CHUNK_WIDTH;
			if (x+y > CHUNK_SIZE || !target->cells[x+y]) { setTerminal('?', j - camera_pos.x, i - camera_pos.y); }
			else { setTerminal(getCellChar(target->cells[x+y]->type), j - camera_pos.x, i - camera_pos.y); }
		}
	}
	return 0;
}
int drawGnome (Gnome * target, int x, int y) {
	setTerminal(target->c, x, y);
	return 0;
}
int drawGnomes () {
	if (!gnomes) { return 1; }
	for (int i = 0; i < gnome_pop; i++) {
		if (!gnomes[i]) { continue; }
		int rX = gnomes[i]->pos.x - camera_pos.x;
		int rY = gnomes[i]->pos.y - camera_pos.y;
		if (rX < 0 || rY < 0 || rX >= VIEW_RANGE_X || rY >= VIEW_RANGE_Y) { continue; }
		drawGnome(gnomes[i], rX, rY);
	}
	return 0;
}


char * getString ()
{
	char buffer[128];
	fgets(buffer, sizeof(buffer), stdin);
	char * p = strchr(buffer, '\n');
	if (p == NULL) { return NULL; }
	else { *p = '\0'; }
	size_t len = strlen(buffer) + 1;
	char * result = (char *) malloc(sizeof(char) * len);
	if (!result) { return NULL; }
	strncpy_s(result, len, buffer, _TRUNCATE);
	return result;
}

int canMoveTo(int x, int y) {
	for (int i = 0; i < gnome_pop; i++) {
		if (!gnomes[i]) { continue; }
		if (gnomes[i]->pos.x == x && gnomes[i]->pos.y == y) { return 0; }
	}
	return 1;
}

void getCommand () {
	char * command = getString();
	if (!strcmp("w", command)) {
		if (player->pos.y <= 0) { message = 2; }
		else if (!canMoveTo(player->pos.x, player->pos.y-1)) { message = 7; }
		else {
			player->pos.y--;
			message = 3;
		}
	}
	else if (!strcmp("s", command)) {
		if (player->pos.y >= CHUNK_HEIGHT - 1) { message = 2; }
		else if (!canMoveTo(player->pos.x, player->pos.y+1)) { message = 7; }
		else {
			player->pos.y++;
			message = 4;
		}
	}
	else if (!strcmp("a", command)) {
		if (player->pos.x <= 0) { message = 2; }
		else if (!canMoveTo(player->pos.x-1, player->pos.y)) { message = 7; }
		else {
			player->pos.x--;
			message = 5;
		}
	}
	else if (!strcmp("d", command)) {
		if (player->pos.x >= CHUNK_WIDTH - 1) { message = 2; }
		else if (!canMoveTo(player->pos.x+1, player->pos.y)) { message = 7; }
		else {
			player->pos.x++;
			message = 6;
		}
	}
	else {
		message = 1;
	}
}


void updateCamera () {
	camera_pos.x = player->pos.x - VIEW_RANGE_X / 2;
	camera_pos.y = player->pos.y - VIEW_RANGE_Y / 2;
	if (camera_pos.x <= 0) { camera_pos.x = 0; }
	if (camera_pos.y <= 0) { camera_pos.y = 0; }
	if (camera_pos.x >= CHUNK_WIDTH) { camera_pos.x = CHUNK_WIDTH - VIEW_RANGE_X; }
	if (camera_pos.y >= CHUNK_HEIGHT) { camera_pos.y = CHUNK_HEIGHT - VIEW_RANGE_Y; }
}

int main ()
{
	// char * test = getString();
	// printf("%s\n", test);
	
	initGlobals();
	player = spawnGnome(1, 1);
	spawnGnome(2, 2);
	player->c = '@';
	camera_pos.x = 0;
	camera_pos.y = 0;
	while (is_running) {
		system("cls");
		printf("player = (%d, %d) | camera = (%d, %d)\n", player->pos.x, player->pos.y, camera_pos.x, camera_pos.y);
		printf("%s", Messages[message]);
		updateCamera();
		drawChunk(region);
		drawGnomes();
		drawTerminal();
		getCommand();
	}
	freeGlobals();
	return 0;
}
