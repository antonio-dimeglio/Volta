#include "../include/vm/GC.hpp"
#include <iostream>

namespace volta::vm {

// ========== Constructor/Destructor ==========

GarbageCollector::GarbageCollector(VM* vm, const GCConfig& config)
    : vm_(vm), config_(config) {
}

GarbageCollector::~GarbageCollector() {
}

// ========== Allocation ==========

StructObject* GarbageCollector::allocateStruct(uint32_t typeId, uint32_t fieldCount) {
    return nullptr;
}

ArrayObject* GarbageCollector::allocateArray(uint32_t length) {
    return nullptr;
}

void* GarbageCollector::allocate(size_t size, ObjectHeader::ObjectKind kind, uint32_t typeId) {
    return nullptr;
}

// ========== Collection ==========

size_t GarbageCollector::collectMinor() {
    return 0;
}

size_t GarbageCollector::collectMajor() {
    return 0;
}

bool GarbageCollector::collectIfNeeded() {
    return false;
}

void GarbageCollector::forceCollect() {
}

// ========== Write Barrier ==========

void GarbageCollector::writeBarrier(void* oldGenObject, void* youngGenObject) {
}

// ========== GC Roots ==========

void GarbageCollector::addRoot(Value* root) {
}

void GarbageCollector::removeRoot(Value* root) {
}

void GarbageCollector::clearRoots() {
}

// ========== Configuration ==========

void GarbageCollector::setConfig(const GCConfig& config) {
}

// ========== Statistics & Debugging ==========

void GarbageCollector::resetStats() {
}

bool GarbageCollector::isYoung(void* obj) const {
    return false;
}

bool GarbageCollector::isOld(void* obj) const {
    return false;
}

void GarbageCollector::printStats(std::ostream& out) const {
}

// ========== Mark Phase ==========

void GarbageCollector::mark(bool fullHeap) {
}

void GarbageCollector::markValue(const Value& value) {
}

void GarbageCollector::markObject(void* obj) {
}

void GarbageCollector::markRoots() {
}

void GarbageCollector::markRememberedSet() {
}

// ========== Sweep Phase ==========

size_t GarbageCollector::sweep(bool fullHeap) {
    return 0;
}

size_t GarbageCollector::sweepGeneration(std::vector<void*>& generation, bool isYoung) {
    return 0;
}

void GarbageCollector::freeObject(void* obj) {
}

// ========== Promotion ==========

void GarbageCollector::promoteObjects() {
}

void GarbageCollector::promoteObject(void* obj) {
}

// ========== Helpers ==========

GCHeader* GarbageCollector::getGCHeader(void* obj) const {
    return nullptr;
}

bool GarbageCollector::shouldCollect() const {
    return false;
}

bool GarbageCollector::shouldCollectMinor() const {
    return false;
}

bool GarbageCollector::shouldCollectMajor() const {
    return false;
}

void GarbageCollector::updateStats(bool wasMinor, size_t bytesFreed, uint64_t durationMs) {
}

} // namespace volta::vm
