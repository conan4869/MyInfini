// Copyright 2014 Isis Innovation Limited and the authors of InfiniTAM

#include "ITMSwappingEngine_CPU.h"
#include "../../DeviceAgnostic/ITMSwappingEngine.h"

using namespace ITMLib::Engine;

template<class TVoxel>
ITMSwappingEngine_CPU<TVoxel,ITMVoxelBlockHash>::ITMSwappingEngine_CPU(void)
{
}

template<class TVoxel>
ITMSwappingEngine_CPU<TVoxel,ITMVoxelBlockHash>::~ITMSwappingEngine_CPU(void)
{
}

template<class TVoxel>
int ITMSwappingEngine_CPU<TVoxel,ITMVoxelBlockHash>::DownloadFromGlobalMemory(ITMScene<TVoxel,ITMVoxelBlockHash> *scene, ITMView *view)
{
	ITMGlobalCache<TVoxel> *globalCache = scene->globalCache;

	ITMHashCacheState *cacheStates = globalCache->GetCacheStates(false);

	int *neededEntryIDs_local = globalCache->GetNeededEntryIDs(false);

	TVoxel *syncedVoxelBlocks_global = globalCache->GetSyncedVoxelBlocks(false);
	bool *hasSyncedData_global = globalCache->GetHasSyncedData(false);
	int *neededEntryIDs_global = globalCache->GetNeededEntryIDs(false);

	int noTotalEntries = globalCache->noTotalEntries;

	int noNeededEntries = 0;
	for (int entryId = 0; entryId < noTotalEntries; entryId++)
	{
		if (noNeededEntries >= SDF_TRANSFER_BLOCK_NUM) break;
		if (cacheStates[entryId].cacheFromHost == 1)
		{
			neededEntryIDs_local[noNeededEntries] = entryId;
			noNeededEntries++;
		}
	}

	// would copy neededEntryIDs_local into neededEntryIDs_global here

	if (noNeededEntries > 0)
	{
		memset(syncedVoxelBlocks_global, 0, noNeededEntries * SDF_BLOCK_SIZE3 * sizeof(TVoxel));
		memset(hasSyncedData_global, 0, noNeededEntries * sizeof(bool));
		for (int i = 0; i < noNeededEntries; i++)
		{
			int entryId = neededEntryIDs_global[i];

			if (globalCache->HasStoredData(entryId))
			{
				hasSyncedData_global[i] = true;
				memcpy(syncedVoxelBlocks_global + i * SDF_BLOCK_SIZE3, globalCache->GetStoredVoxelBlock(entryId), SDF_BLOCK_SIZE3 * sizeof(TVoxel));
			}
		}
	}

	// would copy syncedVoxelBlocks_global and hasSyncedData_global and syncedVoxelBlocks_local and hasSyncedData_local here

	return noNeededEntries;
}

template<class TVoxel>
void ITMSwappingEngine_CPU<TVoxel,ITMVoxelBlockHash>::IntegrateGlobalIntoLocal(ITMScene<TVoxel,ITMVoxelBlockHash> *scene, ITMView *view)
{
	ITMGlobalCache<TVoxel> *globalCache = scene->globalCache;

	ITMHashEntry *hashTable = scene->index.GetEntries();

	ITMHashCacheState *cacheStates = globalCache->GetCacheStates(false);

	TVoxel *syncedVoxelBlocks_local = globalCache->GetSyncedVoxelBlocks(false);
	bool *hasSyncedData_local = globalCache->GetHasSyncedData(false);
	int *neededEntryIDs_local = globalCache->GetNeededEntryIDs(false);

	TVoxel *localVBA = scene->localVBA.GetVoxelBlocks();

	int noNeededEntries = this->DownloadFromGlobalMemory(scene, view);

	int maxW = scene->sceneParams->maxW;

	for (int i = 0; i < noNeededEntries; i++)
	{
		int entryDestId = neededEntryIDs_local[i];

		if (hasSyncedData_local[i])
		{
			TVoxel *srcVB = syncedVoxelBlocks_local + i * SDF_BLOCK_SIZE3;
			TVoxel *dstVB = localVBA + hashTable[entryDestId].ptr * SDF_BLOCK_SIZE3;

			for (int vIdx = 0; vIdx < SDF_BLOCK_SIZE3; vIdx++)
			{
				CombineVoxelInformation<TVoxel::hasColorInformation, TVoxel>::compute(srcVB[vIdx], dstVB[vIdx], maxW);
			}
		}

		cacheStates[entryDestId].cacheFromHost = 2;
	}
}

template<class TVoxel>
void ITMSwappingEngine_CPU<TVoxel,ITMVoxelBlockHash>::SaveToGlobalMemory(ITMScene<TVoxel,ITMVoxelBlockHash> *scene, ITMView *view)
{
	ITMGlobalCache<TVoxel> *globalCache = scene->globalCache;

	ITMHashCacheState *cacheStates = globalCache->GetCacheStates(false);

	ITMHashEntry *hashTable = scene->index.GetEntries();
	uchar *entriesVisibleType = scene->index.GetEntriesVisibleType();

	TVoxel *syncedVoxelBlocks_local = globalCache->GetSyncedVoxelBlocks(false);
	bool *hasSyncedData_local = globalCache->GetHasSyncedData(false);
	int *neededEntryIDs_local = globalCache->GetNeededEntryIDs(false);

	TVoxel *syncedVoxelBlocks_global = globalCache->GetSyncedVoxelBlocks(false);
	bool *hasSyncedData_global = globalCache->GetHasSyncedData(false);
	int *neededEntryIDs_global = globalCache->GetNeededEntryIDs(false);

	TVoxel *localVBA = scene->localVBA.GetVoxelBlocks();
	int *voxelAllocationList = scene->localVBA.GetAllocationList();

	int noTotalEntries = globalCache->noTotalEntries;
	
	int noNeededEntries = 0;
	int noAllocatedVoxelEntries = scene->localVBA.lastFreeBlockId;

	for (int entryDestId = 0; entryDestId < noTotalEntries; entryDestId++)
	{
		if (noNeededEntries >= SDF_TRANSFER_BLOCK_NUM) break;

		int localPtr = hashTable[entryDestId].ptr;
		ITMHashCacheState &cacheState = cacheStates[entryDestId];

		if (cacheState.cacheFromHost == 2 && localPtr >= 0 && entriesVisibleType[entryDestId] == 0)
		{
			TVoxel *localVBALocation = localVBA + localPtr * SDF_BLOCK_SIZE3;

			neededEntryIDs_local[noNeededEntries] = entryDestId;

			hasSyncedData_local[noNeededEntries] = true;
			memcpy(syncedVoxelBlocks_local + noNeededEntries * SDF_BLOCK_SIZE3, localVBALocation, SDF_BLOCK_SIZE3 * sizeof(TVoxel));

			cacheStates[entryDestId].cacheFromHost = 0;

			int vbaIdx = noAllocatedVoxelEntries;
			if (vbaIdx < SDF_BUCKET_NUM - 1)
			{
				noAllocatedVoxelEntries++;
				voxelAllocationList[vbaIdx + 1] = localPtr;
				hashTable[entryDestId].ptr = -1;

				for (int i = 0; i < SDF_BLOCK_SIZE3; i++) localVBALocation[i] = TVoxel();
			}

			noNeededEntries++;
		}
	}

	scene->localVBA.lastFreeBlockId = noAllocatedVoxelEntries;

	// would copy neededEntryIDs_local, hasSyncedData_local and syncedVoxelBlocks_local into *_global here

	if (noNeededEntries > 0)
	{
		for (int entryId = 0; entryId < noNeededEntries; entryId++)
		{
			if (hasSyncedData_global[entryId])
				globalCache->SetStoredData(neededEntryIDs_global[entryId], syncedVoxelBlocks_global + entryId * SDF_BLOCK_SIZE3);
		}
	}
}

template class ITMSwappingEngine_CPU<ITMVoxel,ITMVoxelIndex>;
