/*!
 * \file PropertyVisitorHelpers.h
 * Any FieldVisitor helpers that I think won't fit into FieldVisitor.h will b
 *
 * \author Jeslas
 * \date July 2022
 * \copyright
 *  Copyright (C) Jeslas Pravin, 2022-2023
 *  @jeslaspravin pravinjeslas@gmail.com
 *  License can be read in LICENSE file at this repository's root
 */

#pragma once

#include "CoreObjectsExports.h"
#include "Property/CustomProperty.h"
#include "Visitors/FieldVisitors.h"

class COREOBJECTS_EXPORT PropertyVisitorHelper
{
private:
    PropertyVisitorHelper() = default;

public:
    // For editing pointer in keys only, and any value in values
    template <typename Visitable>
    static void visitEditMapEntriesPtrOnly(const MapProperty *mapProp, void *val, const PropertyInfo & /*propInfo*/, void *userData)
    {
        const IterateableDataRetriever *dataRetriever = static_cast<const IterateableDataRetriever *>(mapProp->dataRetriever);
        const TypedProperty *keyProp = static_cast<const TypedProperty *>(mapProp->keyProp);
        const TypedProperty *valueProp = static_cast<const TypedProperty *>(mapProp->valueProp);

        // map key can be either fundamental or special or struct or class ptr but it can never be custom types
        // Fundamental or special cannot hold pointer to cbe::Object so only struct and pointer is left
        if (PropertyHelper::getUnqualified(keyProp)->type == EPropertyType::ClassType)
        {
            // 2 times the data is needed to copy current and new value, to be removed and added later
            // TODO(Jeslas) : Change to use frame stack allocator
            uint8 *bufferData = (uint8 *)CBEMemory::memAlloc(mapProp->pairSize * (dataRetriever->size(val) * 2 + 1), mapProp->pairAlignment);
            uint8 *tempData = bufferData;
            bufferData += mapProp->pairSize;

            // First - original data, Second - replacement data. both are key value pairs
            std::vector<std::pair<uint8 *, uint8 *>> editedKeys;
            for (auto itrPtr = dataRetriever->createIterator(val); itrPtr->isValid(); itrPtr->iterateFwd())
            {
                void *keyPtr = itrPtr->getElement();
                void *valPtr = static_cast<MapIteratorWrapper *>(itrPtr.get())->value();

                CBEMemory::memZero(tempData, mapProp->pairSize);
                dataRetriever->copyTo(keyPtr, tempData);
                FieldVisitor::visit<Visitable>(keyProp, tempData, userData);
                // If both original and temp new keys are equal then we do not have to replace entry
                if (dataRetriever->equals(keyPtr, tempData))
                {
                    // Copy back key just in case pointer is not used for hashing or equality
                    // checks
                    dataRetriever->copyTo(tempData, keyPtr);
                    // visit the value
                    FieldVisitor::visit<Visitable>(valueProp, valPtr, userData);
                }
                else
                {
                    std::pair<uint8 *, uint8 *> originalToNewPairs;
                    originalToNewPairs.first = bufferData;
                    bufferData += mapProp->pairSize;
                    originalToNewPairs.second = bufferData;
                    bufferData += mapProp->pairSize;
                    // Copy original
                    dataRetriever->copyTo(keyPtr, originalToNewPairs.first);
                    // Copy new key and value
                    dataRetriever->copyTo(tempData, originalToNewPairs.second);
                    // visit the value
                    FieldVisitor::visit<Visitable>(valueProp, originalToNewPairs.second + mapProp->secondOffset, userData);

                    editedKeys.emplace_back(std::move(originalToNewPairs));
                }
            }

            for (const std::pair<uint8 *, uint8 *> &originalToNewPairs : editedKeys)
            {
                dataRetriever->remove(val, originalToNewPairs.first);
                dataRetriever->add(val, originalToNewPairs.second);
            }

            CBEMemory::memFree(tempData);
        }
        else // Only value can have pointer
        {
            for (auto itrPtr = dataRetriever->createIterator(val); itrPtr->isValid(); itrPtr->iterateFwd())
            {
                FieldVisitor::visit<Visitable>(valueProp, static_cast<MapIteratorWrapper *>(itrPtr.get())->value(), userData);
            }
        }
    }

    template <typename Visitable>
    static void visitEditMapEntries(const MapProperty *mapProp, void *val, const PropertyInfo & /*propInfo*/, void *userData)
    {
        const IterateableDataRetriever *dataRetriever = static_cast<const IterateableDataRetriever *>(mapProp->dataRetriever);
        const TypedProperty *keyProp = static_cast<const TypedProperty *>(mapProp->keyProp);
        const TypedProperty *valueProp = static_cast<const TypedProperty *>(mapProp->valueProp);

        // map key can be either fundamental or special or struct or class ptr but it can never be custom types

        // 2 times the data is needed to copy current and new value, to be removed and added later
        // TODO(Jeslas) : Change to use frame stack allocator
        uint8 *bufferData = (uint8 *)CBEMemory::memAlloc(mapProp->pairSize * (dataRetriever->size(val) * 2 + 1), mapProp->pairAlignment);
        uint8 *tempData = bufferData;
        bufferData += mapProp->pairSize;

        // First - original data, Second - replacement data. both are key value pairs
        std::vector<std::pair<uint8 *, uint8 *>> editedKeys;
        for (auto itrPtr = dataRetriever->createIterator(val); itrPtr->isValid(); itrPtr->iterateFwd())
        {
            void *keyPtr = itrPtr->getElement();
            void *valPtr = static_cast<MapIteratorWrapper *>(itrPtr.get())->value();

            CBEMemory::memZero(tempData, mapProp->pairSize);
            dataRetriever->copyTo(keyPtr, tempData);
            FieldVisitor::visit<Visitable>(keyProp, tempData, userData);
            // If both original and temp new keys are equal then we do not have to replace entry
            if (dataRetriever->equals(keyPtr, tempData))
            {
                // Copy back key just in case pointer is not used for hashing or equality
                // checks
                dataRetriever->copyTo(tempData, keyPtr);
                // visit the value
                FieldVisitor::visit<Visitable>(valueProp, valPtr, userData);
            }
            else
            {
                std::pair<uint8 *, uint8 *> originalToNewPairs;
                originalToNewPairs.first = bufferData;
                bufferData += mapProp->pairSize;
                originalToNewPairs.second = bufferData;
                bufferData += mapProp->pairSize;
                // Copy original
                dataRetriever->copyTo(keyPtr, originalToNewPairs.first);
                // Copy new key and value
                dataRetriever->copyTo(tempData, originalToNewPairs.second);
                // visit the value
                FieldVisitor::visit<Visitable>(valueProp, originalToNewPairs.second + mapProp->secondOffset, userData);

                editedKeys.emplace_back(std::move(originalToNewPairs));
            }
        }

        for (const std::pair<uint8 *, uint8 *> &originalToNewPairs : editedKeys)
        {
            dataRetriever->remove(val, originalToNewPairs.first);
            dataRetriever->add(val, originalToNewPairs.second);
        }

        CBEMemory::memFree(tempData);
    }

    /**
     * editing set entries will always leads to removing and inserting changed elements so only one variant is enough(both ptr and value)
     * set key can be either fundamental or special or struct or class ptr but it can never be custom types
     */
    template <typename Visitable>
    static void visitEditSetEntries(const ContainerProperty *setProp, void *val, const PropertyInfo & /*propInfo*/, void *userData)
    {
        const IterateableDataRetriever *dataRetriever = static_cast<const IterateableDataRetriever *>(setProp->dataRetriever);
        const TypedProperty *elementProp = static_cast<const TypedProperty *>(setProp->elementProp);

        // 2 times the data is needed to copy current and new value, to be removed and added later
        uint8 *bufferData
            = (uint8 *)CBEMemory::memAlloc(elementProp->typeInfo->size * (dataRetriever->size(val) * 2 + 1), elementProp->typeInfo->alignment);
        uint8 *tempData = bufferData;
        bufferData += elementProp->typeInfo->size;

        // First - original data, Second - replacement data.
        std::vector<std::pair<uint8 *, uint8 *>> editedKeys;
        for (auto itrPtr = dataRetriever->createIterator(val); itrPtr->isValid(); itrPtr->iterateFwd())
        {
            const void *elemPtr = itrPtr->getConstElement();

            CBEMemory::memZero(tempData, elementProp->typeInfo->size);
            dataRetriever->copyTo(elemPtr, tempData);
            FieldVisitor::visit<Visitable>(elementProp, tempData, userData);
            // If both original and temp new keys are equal then we do not have to replace entry
            if (!dataRetriever->equals(elemPtr, tempData))
            {
                std::pair<uint8 *, uint8 *> originalToNewPairs;
                originalToNewPairs.first = bufferData;
                bufferData += elementProp->typeInfo->size;
                originalToNewPairs.second = bufferData;
                bufferData += elementProp->typeInfo->size;
                // Copy original
                dataRetriever->copyTo(elemPtr, originalToNewPairs.first);
                // Copy new key and value
                dataRetriever->copyTo(tempData, originalToNewPairs.second);

                editedKeys.emplace_back(std::move(originalToNewPairs));
            }
        }

        for (const std::pair<uint8 *, uint8 *> &originalToNewPairs : editedKeys)
        {
            dataRetriever->remove(val, originalToNewPairs.first);
            dataRetriever->add(val, originalToNewPairs.second);
        }

        CBEMemory::memFree(tempData);
    }
};