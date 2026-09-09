#ifndef CTR_NATIVE_NAMESPACE_LEVEL_H
#define CTR_NATIVE_NAMESPACE_LEVEL_H

enum LevelID
{
	DINGO_CANYON = 0, // 0.
	DRAGON_MINES = 1, // 1.
	BLIZZARD_BLUFF,
	CRASH_COVE,
	TIGER_TEMPLE,
	PAPU_PYRAMID,
	ROO_TUBES,
	HOT_AIR_SKYWAY,
	SEWER_SPEEDWAY,
	MYSTERY_CAVES,
	CORTEX_CASTLE,
	N_GIN_LABS,
	POLAR_PASS,
	OXIDE_STATION,
	COCO_PARK,
	TINY_ARENA,
	SLIDE_COLISEUM,
	TURBO_TRACK,
	NITRO_COURT,
	RAMPAGE_RUINS,
	PARKING_LOT,
	SKULL_ROCK,
	THE_NORTH_BOWL,
	ROCKY_ROAD,
	LAB_BASEMENT,
	GEM_STONE_VALLEY,
	N_SANITY_BEACH,
	THE_LOST_RUINS,
	GLACIER_PARK,
	CITADEL_CITY,
	INTRO_RACE_TODAY,
	INTRO_COCO,
	INTRO_TINY,
	INTRO_POLAR,
	INTRO_DINGODILE,
	INTRO_CORTEX,
	INTRO_SPACE,
	INTRO_CRASH,
	INTRO_OXIDE,
	MAIN_MENU_LEVEL,
	ADVENTURE_GARAGE,
	NAUGHTY_DOG_CRATE,
	OXIDE_ENDING,
	OXIDE_TRUE_ENDING,
	CREDITS_CRASH,
	CREDITS_CORTEX,
	CREDITS_TINY,
	CREDITS_COCO,
	CREDITS_N_GIN,
	CREDITS_DINGO,
	CREDITS_POLAR,
	CREDITS_PURA,
	CREDITS_PINSTRIPE,
	CREDITS_PAPU,
	CREDITS_ROO,
	CREDITS_JOE,
	CREDITS_TROPY,
	CREDITS_PENTA,
	CREDITS_FAKE_CRASH,
	CREDITS_OXIDE,
	CREDITS_AMI,
	CREDITS_ISABELLA,
	CREDITS_LIZ,
	CREDITS_MEGUMI,
	SCRAPBOOK
};

enum AdventureSyntheticLevelID
{
	ADVENTURE_CUP_SYNTHETIC_LEVEL_ID_BASE = 100,
};

enum TerrainType
{
	TERRAIN_ASPHALT,
	TERRAIN_DIRT,
	TERRAIN_GRASS,
	TERRAIN_WOOD,
	TERRAIN_WATER,
	TERRAIN_STONE,
	TERRAIN_ICE,
	TERRAIN_TRACK,
	TERRAIN_ICY_ROAD,
	TERRAIN_SNOW,
	TERRAIN_NONE,
	TERRAIN_HARDPACK,
	TERRAIN_METAL,
	TERRAIN_FASTWATER,
	TERRAIN_MUD,
	TERRAIN_SIDESLIP,
	TERRAIN_RIVERASPHALT,
	TERRAIN_STEAMASPHALT,
	TERRAIN_OCEANASPHALT,
	TERRAIN_SLOWGRASS,
	TERRAIN_SLOWDIRT
};

// transparent parameter of getTPage()
enum BlendMode
{
	// 50% transparency
	TRANS_50 = 0,

	// additive blending
	ADD = 1,

	// subtractive blending
	SUBTRACT = 2,

	// additive blending on 25% transparency
	ADD_25 = 3
};

// textures used for a quad in a quadblock
// ctr-tools only rips the "near" and "mosaic" texture levels, but the idea is that middle and far are to use lower-quality textures
struct IconGroup4
{
	struct TextureLayout far;
	struct TextureLayout middle;
	struct TextureLayout near;
	struct TextureLayout mosaic;
};

struct AnimTex
{
	// 0x0
	// pointer to IconGroup4 struct to be animated
	// cycles through the entirety of ptrarray
	int *ptrActiveTex;

	// 0x4
	s16 numFrames;

	// 0x6
	s16 frameOffset;

	// 0x8
	s16 frameSkip;

	// 0xA
	s16 frameCurr;

	// 0xC
	// size = numFrames
	// After this array is the next AnimTex
	// struct IconGroup4* ptrarray[0];
};

// NOTE: (u32)x truncates real pointers on 64-bit (Switch/AArch64); use a byte
// pointer for the offset instead (identical codegen/addresses on 32-bit).
#define ANIMTEX_GETARRAY(x) (struct IconGroup4 **)((u8 *)(x) + sizeof(struct AnimTex))

struct PVS
{
	int *visLeafSrc;
	int *visFaceSrc;
	struct Instance **visInstSrc;

	// either OVert or SCVert
	int *visExtraSrc;
};

typedef s16 BspChildId;

enum
{
	QUADBLOCK_FLAG_REFLECT_SPLIT_LINE_1 = 0x0001,
	QUADBLOCK_FLAG_LOW_GRAVITY = 0x0002,
	QUADBLOCK_FLAG_REFLECT_SPLIT_LINE_0 = 0x0004,
	QUADBLOCK_FLAG_NO_COLLISION_RESPONSE = 0x0010,
	QUADBLOCK_FLAG_TRIGGER = 0x0040,
	QUADBLOCK_FLAG_ENGINE_ECHO = 0x0080,
	QUADBLOCK_FLAG_KILL_PLANE = 0x0200,
	QUADBLOCK_FLAG_DOOR = 0x0400,
	QUADBLOCK_FLAG_CAMERA_SEARCH = 0x0800,
	QUADBLOCK_FLAG_GROUND = 0x1000,
	QUADBLOCK_FLAG_COLLISION_SURFACE = 0x2000,
	QUADBLOCK_FLAG_NO_CAMERA_RESPAWN_PROBE = 0x4000,
	QUADBLOCK_FLAG_SKIP_WATER_LIST = 0x8000,
};
typedef u16 QuadBlockFlags;

enum QuadBlockTriNormalDividendIndex
{
	QUADBLOCK_TRI_NORMAL_DIVIDEND_HI_0 = 0,
	QUADBLOCK_TRI_NORMAL_DIVIDEND_HI_1 = 1,
	QUADBLOCK_TRI_NORMAL_DIVIDEND_HI_2 = 2,
	QUADBLOCK_TRI_NORMAL_DIVIDEND_HI_3 = 3,
	QUADBLOCK_TRI_NORMAL_DIVIDEND_HI_4 = 4,
	QUADBLOCK_TRI_NORMAL_DIVIDEND_HI_5 = 5,
	QUADBLOCK_TRI_NORMAL_DIVIDEND_HI_6 = 6,
	QUADBLOCK_TRI_NORMAL_DIVIDEND_HI_7 = 7,
	QUADBLOCK_TRI_NORMAL_DIVIDEND_LO_0 = 8,
	QUADBLOCK_TRI_NORMAL_DIVIDEND_LO_1 = 9,
};

#define QUADBLOCK_DRAW_ORDER_LOW_DOUBLE_SIDED 0x80000000u

struct QuadBlock
{
	// 0x0
	u16 index[9];

	// 0x12
	QuadBlockFlags quadFlags;

	// 0x14
	// Packed draw-order byte, four 5-bit face-mode fields, and a double-sided bit.
	/*
	    drawOrderLow |
	    faceFlags[0].packedValue << (8 + 0 * 5) |
	    faceFlags[1].packedValue << (8 + 1 * 5) |
	    faceFlags[2].packedValue << (8 + 2 * 5) |
	    faceFlags[3].packedValue << (8 + 3 * 5) |
	    (doubleSided ? 1 : 0) << 31)
	*/
	/*
	    //where 5 bits of faceFlags are
	    3 bits Rotation = x & 7;
	    2 bits faceMode = (x >> 3) & 3;
	*/
	u32 draw_order_low;


	// 0x18
	// 4 bytes, byte per mid quad
	u32 draw_order_high;

	// 0x1c
	// used for the textures of all 4 quads in a medium-level quadblock
	// usually points to IconGroup4, but can also point to AnimTex structs
	// member 0 is 0,0 in xy, 1 is 1,0, 2 is 0,1, 3 is 1,1
	void *ptr_texture_mid[4];

	// 0x2c
	struct BoundingBox bbox;

	// 0x38
	u8 terrain_type;
	char weather_intensity;
	char weather_vanishRate;
	s8 mulNormVecY; // -127 for AntiGrav Sewer

	// 0x3C
	s16 blockID;

	// 0x3E
	// used for checkpoint progression
	// and also respawns
	u8 checkpointIndex;

	char triNormalVecBitShift;

	// 0x40
	// used for the texture of a quad in low level of detail
	// the same as ptr_texture_mid, just not as an array
	void *ptr_texture_low;

	// 0x44
	struct PVS *pvs;

	// 0x48
	// explained in FUN_8001f2dc
	s16 triNormalVecDividend[10];

	// full struct is 0x5c bytes large
};

CTR_STATIC_ASSERT(sizeof(struct QuadBlock) == 0x5c);
CTR_STATIC_ASSERT(offsetof(struct QuadBlock, index) == 0x0);
CTR_STATIC_ASSERT(sizeof(((struct QuadBlock *)0)->index[0]) == 0x2);
CTR_STATIC_ASSERT(sizeof(QuadBlockFlags) == 0x2);
CTR_STATIC_ASSERT(QUADBLOCK_FLAG_REFLECT_SPLIT_LINE_1 == 0x0001);
CTR_STATIC_ASSERT(QUADBLOCK_FLAG_LOW_GRAVITY == 0x0002);
CTR_STATIC_ASSERT(QUADBLOCK_FLAG_REFLECT_SPLIT_LINE_0 == 0x0004);
CTR_STATIC_ASSERT(QUADBLOCK_FLAG_NO_COLLISION_RESPONSE == 0x0010);
CTR_STATIC_ASSERT(QUADBLOCK_FLAG_TRIGGER == 0x0040);
CTR_STATIC_ASSERT(QUADBLOCK_FLAG_ENGINE_ECHO == 0x0080);
CTR_STATIC_ASSERT(QUADBLOCK_FLAG_KILL_PLANE == 0x0200);
CTR_STATIC_ASSERT(QUADBLOCK_FLAG_DOOR == 0x0400);
CTR_STATIC_ASSERT(QUADBLOCK_FLAG_CAMERA_SEARCH == 0x0800);
CTR_STATIC_ASSERT(QUADBLOCK_FLAG_GROUND == 0x1000);
CTR_STATIC_ASSERT(QUADBLOCK_FLAG_COLLISION_SURFACE == 0x2000);
CTR_STATIC_ASSERT(QUADBLOCK_FLAG_NO_CAMERA_RESPAWN_PROBE == 0x4000);
CTR_STATIC_ASSERT(QUADBLOCK_FLAG_SKIP_WATER_LIST == 0x8000);
CTR_STATIC_ASSERT(offsetof(struct QuadBlock, quadFlags) == 0x12);
CTR_STATIC_ASSERT(offsetof(struct QuadBlock, checkpointIndex) == 0x3e);
CTR_STATIC_ASSERT(offsetof(struct QuadBlock, ptr_texture_low) == 0x40);
CTR_STATIC_ASSERT(offsetof(struct QuadBlock, pvs) == 0x44);
CTR_STATIC_ASSERT(offsetof(struct QuadBlock, triNormalVecDividend) == 0x48);
CTR_STATIC_ASSERT(QUADBLOCK_TRI_NORMAL_DIVIDEND_HI_0 == 0);
CTR_STATIC_ASSERT(QUADBLOCK_TRI_NORMAL_DIVIDEND_HI_7 == 7);
CTR_STATIC_ASSERT(QUADBLOCK_TRI_NORMAL_DIVIDEND_LO_0 == 8);
CTR_STATIC_ASSERT(QUADBLOCK_TRI_NORMAL_DIVIDEND_LO_1 == 9);
CTR_STATIC_ASSERT(QUADBLOCK_DRAW_ORDER_LOW_DOUBLE_SIDED == 0x80000000u);

// BSP box that contains geometry
struct BSP
{
	// 0x0
	u16 flag;

	// 0x2
	s16 id;

	// 0x4
	struct BoundingBox box;

	// 0x10

	// determined by flag
	// 0: vis_data_branch
	// 1: vis_data_leaf
	union
	{
		// 0x10
		struct
		{
			// 0x10
			// x, y, z, [?]
			s16 axis[4];

			// 0x18
			// leftChildID, rightChildID, [?], [?]
			BspChildId childID[4];

		} branch;

		// 0x10
		struct
		{
			// 0x10
			int unk1;

			// 0x14
			// the code keeps looping through the
			// array until it finds a 4-byte 0x00000000
			// to determine end of list
			struct BSP *bspHitboxArray;

			// 0x18
			int numQuads;

			// 0x1C
			struct QuadBlock *ptrQuadBlockArray;
		} leaf;

		// 0x10
		struct
		{
			// 0x10
			SVec3 center;

			// 0x16
			s16 radius;

			// 0x18
			s16 unk18;

			// 0x1A
			s16 unk1A;

			// 0x1C
			// These are always InstDef, not converted to Instance
			struct InstDef *instDef;
		} hitbox;

	} data;

	// 0x20 bytes large
};

enum
{
	BSP_CHILD_ID_INDEX_MASK = 0x3fff,
	BSP_CHILD_ID_LEAF_FLAG = 0x4000,
	BSP_CHILD_ID_NONE = 0xffff,
};
typedef u16 BspChildIdEncoding;

enum
{
	BSP_NODE_FLAG_LEAF = 0x0001,
};
typedef u16 BspNodeFlag;

enum
{
	BSP_RENDER_LEAF_FLAG_4X1 = 0x0008,
	BSP_RENDER_LEAF_FLAG_4X2 = 0x0010,
	BSP_RENDER_LEAF_FLAG_DYNAMIC_SUBDIV = 0x0020,
	BSP_RENDER_LEAF_FLAG_4X4 = 0x0080,
};
typedef u16 BspRenderLeafFlag;

enum
{
	BSP_HITBOX_LINC_USES_INSTDEF = 0x10,
	BSP_HITBOX_CHECK_Y_RANGE = 0x20,
	BSP_HITBOX_USE_Y_AXIS = 0x40,
	BSP_HITBOX_COLLIDABLE = 0x80,
};
typedef u16 BspHitboxFlag;

enum
{
	BSP_LEAF_FLAG_WATER = 0x2,
};
typedef u16 BspLeafFlag;

enum BspHitboxClass
{
	BSP_HITBOX_CLASS_TOUCH = 4,
};

CTR_STATIC_ASSERT(sizeof(struct BSP) == 0x20);
CTR_STATIC_ASSERT(sizeof(BspChildId) == 0x2);
CTR_STATIC_ASSERT(sizeof(BspChildIdEncoding) == 0x2);
CTR_STATIC_ASSERT(BSP_CHILD_ID_INDEX_MASK == 0x3fff);
CTR_STATIC_ASSERT(BSP_CHILD_ID_LEAF_FLAG == 0x4000);
CTR_STATIC_ASSERT(BSP_CHILD_ID_NONE == 0xffff);
CTR_STATIC_ASSERT(sizeof(BspNodeFlag) == 0x2);
CTR_STATIC_ASSERT(BSP_NODE_FLAG_LEAF == 0x0001);
CTR_STATIC_ASSERT(sizeof(BspRenderLeafFlag) == 0x2);
CTR_STATIC_ASSERT(BSP_RENDER_LEAF_FLAG_4X1 == 0x0008);
CTR_STATIC_ASSERT(BSP_RENDER_LEAF_FLAG_4X2 == 0x0010);
CTR_STATIC_ASSERT(BSP_RENDER_LEAF_FLAG_DYNAMIC_SUBDIV == 0x0020);
CTR_STATIC_ASSERT(BSP_RENDER_LEAF_FLAG_4X4 == 0x0080);
CTR_STATIC_ASSERT(sizeof(BspHitboxFlag) == 0x2);
CTR_STATIC_ASSERT(BSP_HITBOX_LINC_USES_INSTDEF == 0x10);
CTR_STATIC_ASSERT(BSP_HITBOX_CHECK_Y_RANGE == 0x20);
CTR_STATIC_ASSERT(BSP_HITBOX_USE_Y_AXIS == 0x40);
CTR_STATIC_ASSERT(BSP_HITBOX_COLLIDABLE == 0x80);
CTR_STATIC_ASSERT(sizeof(BspLeafFlag) == 0x2);
CTR_STATIC_ASSERT(BSP_LEAF_FLAG_WATER == 0x2);
CTR_STATIC_ASSERT(offsetof(struct BSP, flag) == 0x0);
CTR_STATIC_ASSERT(offsetof(struct BSP, id) == 0x2);
CTR_STATIC_ASSERT(offsetof(struct BSP, box) == 0x4);
CTR_STATIC_ASSERT(offsetof(struct BSP, data.branch.childID) == 0x18);
CTR_STATIC_ASSERT(offsetof(struct BSP, data.hitbox.center) == 0x10);
CTR_STATIC_ASSERT(offsetof(struct BSP, data.hitbox.radius) == 0x16);
CTR_STATIC_ASSERT(offsetof(struct BSP, data.hitbox.instDef) == 0x1C);

struct VisMemBspListNode
{
	struct VisMemBspListNode *next;
	struct BSP *bsp;
};

CTR_STATIC_ASSERT(sizeof(struct VisMemBspListNode) == 8);

struct LevVertex
{
	// 0x0
	SVec3 pos;

	// 0x6
	// COLL copies this into BspSearchVertex.normalAxis before recomputing triangle planes.
	u16 flags;

	// 0x8
	u8 color_hi[4];

	// 0xC
	u8 color_lo[4];

	// 0x10 bytes large
};

// scenery vertex
struct SCVert
{
	struct LevVertex *v;
	int offset_pos_xy;
	int offset_pos_zw;
	int offset_color_rgba;
};

// ocean vertex
struct OVert
{
	// int & 0x003f
	// int & 0x0fc0
	// int & 0xf000

	// 2 bytes large,
	// always in pairs of two
	s16 data[2];
};

struct WaterVert
{
	struct LevVertex *v;
	struct OVert *w;
};

// used for rain and snow particles
struct RainBuffer
{
	// 0x0 (0x1a40)
	int numParticles_curr;

	// 0x4 (0x1a44)
	// if zero, then there is no max
	s16 numParticles_max;
	s16 vanishRate;

	// 0x8
	char unk_4[0x10];

	// 0x18 (0x1a58)
	SVec3 cameraPos;

	// 0x1E
	s16 unk_22;

	// 0x20
	// controls top color of particles
	u32 colorRGBA_top;

	// 0x24
	// controls bottom color of particles
	u32 colorRGBA_bottom;

	// 0x28
	// controls how particles are drawn
	int fillMode;

	// 0x2C
	int offsetOT;

	// 0x30 -- size of struct
};

struct VisMem
{
	// 4 of each for number of players

	// 0x00-0x3F are all DST
	// 0x40-0x7F are all SRC
	// SRC constantly copies to DST
	// DST never changes
	// SRC always changes based on CamDC
	// why is the copy needed?

	// 0x00-0x0F
	// list of BSP leaf nodes
	// size = numLeaf/32
	int *visLeafList[4]; // real ND name

	// 0x10-0x1F
	// bit index quadblock visibility
	// size = numQuadBlock/32 bytes
	int *visFaceList[4]; // real ND name

	// 0x20-0x2F
	// bit index ocean visibility
	int *visOVertList[4]; // real ND name

	// 0x30-0x3F
	// bit index scenery visibility
	int *visSCVertList[4]; // real ND name

	// 0x40-0x4F
	int *visLeafSrc[4]; // copies to other

	// 0x50-0x5F
	int *visFaceSrc[4]; // copies to other

	// 0x60-0x6F
	int *visOVertSrc[4]; // copies to other

	// 0x70-0x7F
	int *visSCVertSrc[4]; // copies to other

	// 0x80-0x8F
	// size = 8 * numBspNodes,
	// this is the memory where RenderLists exist,
	// allows every BSP to link to another BSP
	struct VisMemBspListNode *bspList[4];
};

struct mesh_info
{
	// 0x0
	int numQuadBlock;

	// 0x4
	int numVertex;

	// 0x8
	int unk1;

	// 0xC
	struct QuadBlock *ptrQuadBlockArray;

	// 0x10
	struct LevVertex *ptrVertexArray;

	// 0x14
	int unk2;

	// 0x18
	struct BSP *bspRoot;

	// 0x1C
	int numBspNodes;

	// 0x20 bytes large
};

enum ST1
{
	ST1_MAP = 0,
	ST1_SPAWN = 1,
	ST1_CAMERA_EOR,
	ST1_CAMERA_PATH,
	ST1_NTROPY,
	ST1_NOXIDE,
	ST1_CREDITS
};

struct SpawnType1
{
	int count;

	// void* pointers[0];
};
// NOTE: (u32)x truncates real pointers on 64-bit (Switch/AArch64); use a byte
// pointer for the offset instead (identical codegen/addresses on 32-bit).
#define ST1_GETPOINTERS(x) (void **)((u8 *)(x) + sizeof(struct SpawnType1))

struct SpawnPosRot
{
	SVec3 pos;
	SVec3 rot;
};

struct SpawnType2
{
	int numCoords;
	union
	{
		s16 *posCoords;
		SVec3 *positions;
		struct SpawnPosRot *posRot;
	};
};

// per-quadblock checkpoint node
// each node holds the amount of distance to finish line as well as the index of the nodes that go before or after it
// and position values, which are used for respawns as well as the warp orb path
// they're also used for the track videos on the main menu!
struct CheckpointNode
{
	// 0x0
	SVec3 pos;

	// 0x6
	u16 distToFinish;

	// 0x8
	u8 nextIndex_forward;
	u8 nextIndex_left;
	u8 nextIndex_backward;
	u8 nextIndex_right;

	// 0xC -- size
};

struct Stars
{
	s16 numStars;
	s16 spread;
	s16 seed;
	s16 distance;
};

struct SkyboxFace
{
	u16 A; // ABC stores offsets to add to ptrVerts. divide by 12 if you need an index
	u16 B;
	u16 C;
	u16 D; // this is uh, ot ptr increment? always 0 in the files
};

struct ShortVertex
{
	SVECTOR Position; // this is padded 2*4
	CVECTOR Color;
};

#define NUM_SKYBOX_SEGMENTS 8
struct Skybox
{
	int numVertex;
	struct ShortVertex *ptrVertex;

	s16 numFaces[NUM_SKYBOX_SEGMENTS];
	struct SkyboxFace *ptrFaces[NUM_SKYBOX_SEGMENTS];

	// struct SkyboxFace allFaces[0];
};
#define SKY_GETFACES(x) ((u32)x + sizeof(struct Skybox))

struct LevTexLookup
{
	int numIcon;
	struct Icon *firstIcon;
	int numIconGroup;
	struct IconGroup **firstIconGroupPtr;
};

// NOTE(aalhendi): Every pointer-typed field below is stored as a plain `u32`
// ("_slot" suffix) instead of a real pointer. Retail stores this whole
// struct as a byte-for-byte file format (LOAD_RunPtrMap patches specific
// 4-byte offsets in the raw file bytes with no knowledge of any C struct),
// so each pointer slot must stay exactly 4 bytes, at exactly its retail
// offset. A real pointer field would need 8-byte alignment on 64-bit
// Switch, which inserts padding and shifts every subsequent field off its
// file-format offset - corrupting every read after the first pointer field.
// `u32` only needs 4-byte alignment, so no padding is inserted and every
// offset below matches retail exactly (verified against the existing
// CTR_STATIC_ASSERT offset checks at the bottom of this file).
// Use the Level_Get*/Level_Set* inline accessors (below the struct) to
// convert to/from a real pointer - never dereference a "_slot" field
// directly.
struct Level
{
	// 0x0
	// pointer to mesh info
	u32 ptr_mesh_info_slot;

	// 0x4
	// pointer to skybox (struct not yet known)
	u32 ptr_skybox_slot;

	// 0x8
	// pointer to array of animated texture structs
	u32 ptr_anim_tex_slot;

	// 0xc
	// number of model instances in the level
	// (i.e. every single box, fruit, etc.)
	u32 numInstances;

	// 0x10
	// points to the 1st entry of the array of InstDefs
	// (whatever they are)
	u32 ptrInstDefs_slot;

	// 0x14
	// number of actual models
	u32 numModels;

	// 0x18
	// pointer to the array of pointers to models
	u32 ptrModelsPtrArray_slot;

	// 0x1c
	// unknown, extra bsp region
	u32 unk3_slot;

	// 0x20
	// unknown, extra bsp region
	u32 unk4_slot;

	// 0x24
	// pointer to the array of pointers to model instances (?)
	// converts back and forth, Instance to InstDef
	u32 ptrInstDefPtrArray_slot;

	// 0x28
	// default packed OVert visibility bitset
	u32 visOVertSrc_slot;

	// 0x2c
	// assumed to be reserved
	u32 null1_slot;

	// 0x30
	// assumed to be reserved
	u32 null2_slot;

	// 0x34
	// number of vertices treated as water
	int numWaterVertices;

	// 0x38
	// pointer to array of water entries
	u32 ptr_water_slot;

	// 0x3c
	// leads to the icon pack header
	u32 levTexLookup_slot;

	// 0x40
	// leads to the icon pack data
	u32 ptr_named_tex_array_slot;

	// 0x44
	// pointer to environment map texture layout, used by water rendering
	u32 ptr_tex_waterEnvMap_slot;

	// 0x48
	// used for additional skybox gradients (e.g. papu's pyramid)
	struct SkyboxGlowGradient
	{
		s16 pointFrom;
		s16 pointTo;
		u32 colorFrom;
		u32 colorTo;
	} glowGradient[3];

	// 0x6c
	// array of 8 starting locations
	struct
	{
		SVec3 pos;
		SVec3 rot;
	} DriverSpawn[8];

	// 0xCC -- next
	// unknown, extra bsp regions
	u32 unk_Lev_CC_slot;
	u32 unk_Lev_D0_slot;

	// 0xD4
	// assumed to be a pointer to low textures array, there is no number of entries though
	u32 ptrLowTexArray_slot;

	// 0xD8
	// Used in Coco Park, encoded as Blue
	u32 clearColorRGBA;

	// 0xDC
	// toggles some level stuff
	// & 1 = enables glowGradient skybox gradients
	// & 2 = mask grab when underwater
	// & 4 = toggles between water and animated vertices?
	u32 configFlags;

	// 0xE0
	// pointer to string, date, assumed bsp compilation start
	u32 build_start_slot;

	// 0xE4
	// pointer to string, date, assumed bsp compilation end
	u32 build_end_slot;

	// 0xE8
	// pointer to string, assumed build type
	u32 build_type_slot;

	// 0xEC
	char unk_EC[0x18];

	// 0x104
	// used for rain and snow
	struct RainBuffer rainBuffer;

	// 0x134
	u32 ptrSpawnType1_slot;

	// spawn_arrays2 is for things
	// like Seal, Minecart, etc,
	// series of SVec3 positions

	// 0x138
	int numSpawnType2;

	// 0x13C
	u32 ptrSpawnType2_slot;

	// spawn_arrays is for things
	// N Gin Labs barrel, Snowball,
	// series of SpawnPosRot records

	// 0x140
	int numSpawnType2_PosRot;

	// 0x144
	u32 ptrSpawnType2_PosRot_slot;

	// restart_points is for respawning
	// driver on track after falling off

	// 0x148
	int cnt_restart_points;

	// 0x14C
	u32 ptr_restart_points_slot;

	// 0x150
	char unk_150[0x10];

	// 0x160
	// for gradient clears
	struct
	{
		char rgb[3];
		char enable;

		// 0 - top half
		// 1 - bottom half
		// 2 - gradient
	} clearColor[3];

	// 0x16C
	int unk_16C;

	// 0x170
	// default packed SCVert visibility bitset
	u32 visSCVertSrc_slot;

	// 0x174
	int numSCVert;

	// 0x178
	u32 ptrSCVert_slot;

	// 0x17c - 0x182
	struct Stars stars;

	// split-lines
	// for ice, mud, water
	// 0x184
	// 0x186
	// char splitLines[4]; //probably 2 shorts, not char[4]
	s16 splitLines[2];

	// 0x188
	u32 LevNavTable_slot;

	// 0x18C
	union
	{
		int unk_18C;
		struct
		{
			u8 jumpVerticalSpeedCap;
			u8 unk_18D;
			u8 unk_18E;
			u8 unk_18F;
		};
	};

	// 0x190
	u32 visMem_slot;

	char footer[0x60];
};

#if defined(__SWITCH__)
#include <platform/native_memory.h>
#define LEVEL_PTR_ACCESSOR(fieldName, ptrType)                                                                                                     \
	force_inline ptrType Level_Get##fieldName(const struct Level *lev)                                                                             \
	{                                                                                                                                                \
		return (ptrType)NativeMempack_ReconstructPointer(lev->fieldName##_slot);                                                                   \
	}                                                                                                                                                \
	force_inline void Level_Set##fieldName(struct Level *lev, ptrType value)                                                                        \
	{                                                                                                                                                \
		lev->fieldName##_slot = NativeMempack_TruncatePointer(value);                                                                               \
	}
#else
#define LEVEL_PTR_ACCESSOR(fieldName, ptrType)                                                                                                     \
	force_inline ptrType Level_Get##fieldName(const struct Level *lev)                                                                             \
	{                                                                                                                                                \
		return (ptrType)(uintptr_t)lev->fieldName##_slot;                                                                                          \
	}                                                                                                                                                \
	force_inline void Level_Set##fieldName(struct Level *lev, ptrType value)                                                                        \
	{                                                                                                                                                \
		lev->fieldName##_slot = (u32)(uintptr_t)value;                                                                                              \
	}
#endif

LEVEL_PTR_ACCESSOR(ptr_mesh_info, struct mesh_info *)
LEVEL_PTR_ACCESSOR(ptr_skybox, struct Skybox *)
LEVEL_PTR_ACCESSOR(ptr_anim_tex, struct AnimTex *)
LEVEL_PTR_ACCESSOR(ptrInstDefs, struct InstDef *)
LEVEL_PTR_ACCESSOR(ptrModelsPtrArray, struct Model **)
LEVEL_PTR_ACCESSOR(unk3, void *)
LEVEL_PTR_ACCESSOR(unk4, void *)
LEVEL_PTR_ACCESSOR(ptrInstDefPtrArray, struct InstDef **)
LEVEL_PTR_ACCESSOR(visOVertSrc, int *)
LEVEL_PTR_ACCESSOR(null1, void *)
LEVEL_PTR_ACCESSOR(null2, void *)
LEVEL_PTR_ACCESSOR(ptr_water, struct WaterVert *)
LEVEL_PTR_ACCESSOR(levTexLookup, struct LevTexLookup *)
LEVEL_PTR_ACCESSOR(ptr_named_tex_array, struct Icon *)
LEVEL_PTR_ACCESSOR(ptr_tex_waterEnvMap, struct TextureLayout *)
LEVEL_PTR_ACCESSOR(unk_Lev_CC, void *)
LEVEL_PTR_ACCESSOR(unk_Lev_D0, void *)
LEVEL_PTR_ACCESSOR(ptrLowTexArray, void *)
LEVEL_PTR_ACCESSOR(build_start, char *)
LEVEL_PTR_ACCESSOR(build_end, char *)
LEVEL_PTR_ACCESSOR(build_type, char *)
LEVEL_PTR_ACCESSOR(ptrSpawnType1, struct SpawnType1 *)
LEVEL_PTR_ACCESSOR(ptrSpawnType2, struct SpawnType2 *)
LEVEL_PTR_ACCESSOR(ptrSpawnType2_PosRot, struct SpawnType2 *)
LEVEL_PTR_ACCESSOR(ptr_restart_points, struct CheckpointNode *)
LEVEL_PTR_ACCESSOR(visSCVertSrc, int *)
LEVEL_PTR_ACCESSOR(ptrSCVert, struct SCVert *)
LEVEL_PTR_ACCESSOR(LevNavTable, struct NavHeader **)
LEVEL_PTR_ACCESSOR(visMem, struct VisMem *)

#undef LEVEL_PTR_ACCESSOR

CTR_STATIC_ASSERT(sizeof(struct RainBuffer) == 0x30);
CTR_STATIC_ASSERT(sizeof(struct SpawnPosRot) == 0xc);
CTR_STATIC_ASSERT(offsetof(struct SpawnPosRot, pos) == 0x0);
CTR_STATIC_ASSERT(offsetof(struct SpawnPosRot, rot) == 0x6);
CTR_STATIC_ASSERT(sizeof(struct SpawnType2) == 0x8);
CTR_STATIC_ASSERT(offsetof(struct SpawnType2, numCoords) == 0x0);
CTR_STATIC_ASSERT(offsetof(struct SpawnType2, posCoords) == 0x4);
CTR_STATIC_ASSERT(offsetof(struct SpawnType2, positions) == 0x4);
CTR_STATIC_ASSERT(offsetof(struct SpawnType2, posRot) == 0x4);
CTR_STATIC_ASSERT(sizeof(struct VisMem) == 0x90);
CTR_STATIC_ASSERT(offsetof(struct VisMem, visSCVertList) == 0x30);
CTR_STATIC_ASSERT(offsetof(struct VisMem, visLeafSrc) == 0x40);
CTR_STATIC_ASSERT(offsetof(struct VisMem, visFaceSrc) == 0x50);
CTR_STATIC_ASSERT(offsetof(struct VisMem, visOVertSrc) == 0x60);
CTR_STATIC_ASSERT(offsetof(struct VisMem, visSCVertSrc) == 0x70);
CTR_STATIC_ASSERT(offsetof(struct VisMem, bspList) == 0x80);
CTR_STATIC_ASSERT(sizeof(struct SCVert) == 0x10);
CTR_STATIC_ASSERT(offsetof(struct Level, jumpVerticalSpeedCap) == 0x18C);
CTR_STATIC_ASSERT(offsetof(struct Level, visOVertSrc) == 0x28);
CTR_STATIC_ASSERT(offsetof(struct Level, visSCVertSrc) == 0x170);
CTR_STATIC_ASSERT(offsetof(struct Level, ptrSCVert) == 0x178);

#endif
