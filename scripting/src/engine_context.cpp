#include "../include/engine_context.h"

#include "../include/cylinder_bank_node.h"

es_script::EngineContext::EngineContext() {
    /* void */
}

es_script::EngineContext::~EngineContext() {
    /* void */
}

void es_script::EngineContext::addHead(CylinderHeadNode *node, CylinderHead *head) {
    m_heads[node] = head;
}

CylinderHead *es_script::EngineContext::getHead(CylinderHeadNode *head) {
    auto it = m_heads.find(head);
    if (it != m_heads.end()) {
        return it->second;
    }
    else return nullptr;
}

void es_script::EngineContext::addBank(CylinderBankNode *node, CylinderBank *bank) {
    m_banks[node] = bank;
}

CylinderBank *es_script::EngineContext::getBank(CylinderBankNode *bank) const {
    auto it = m_banks.find(bank);
    if (it != m_banks.end()) {
        return it->second;
    }
    else return nullptr;
}

void es_script::EngineContext::addRodJournal(RodJournalNode *node, int index) {
    m_rodJournals[node] = index;
}

int es_script::EngineContext::getRodJournalIndex(RodJournalNode *node) const {
    auto it = m_rodJournals.find(node);
    if (it != m_rodJournals.end()) {
        return it->second;
    }
    else return -1;
}

void es_script::EngineContext::addIntake(IntakeNode *node, Intake *intake) {
    m_intakes[node] = intake;
}

Intake *es_script::EngineContext::getIntake(IntakeNode *intake) const {
    auto it = m_intakes.find(intake);
    if (it != m_intakes.end()) {
        return it->second;
    }
    else return nullptr;
}

void es_script::EngineContext::addExhaust(
    ExhaustSystemNode *node,
    ExhaustSystem *exhaust)
{
    m_exhaustSystems[node] = exhaust;
}

ExhaustSystem *es_script::EngineContext::getExhaust(ExhaustSystemNode *exhaust) const {
    auto it = m_exhaustSystems.find(exhaust);
    if (it != m_exhaustSystems.end()) {
        return it->second;
    }
    else return nullptr;
}

void es_script::EngineContext::addFunction(FunctionNode *node, Function *function) {
    m_functions[node] = function;
}

Function *es_script::EngineContext::getFunction(FunctionNode *exhaust) const {
    auto it = m_functions.find(exhaust);
    if (it != m_functions.end()) {
        return it->second;
    }
    else return nullptr;
}

void es_script::EngineContext::addImpulseResponse(
    ImpulseResponseNode *node,
    ImpulseResponse *impulse)
{
    m_impulseResponses[node] = impulse;
}

ImpulseResponse *es_script::EngineContext::getImpulseResponse(
    ImpulseResponseNode *node) const
{
    auto it = m_impulseResponses.find(node);
    if (it != m_impulseResponses.end()) {
        return it->second;
    }
    else return nullptr;
}

void es_script::EngineContext::addCrankshaft(
    CrankshaftNode *node,
    Crankshaft *crankshaft)
{
    m_crankshafts[node] = crankshaft;
}

Crankshaft *es_script::EngineContext::getCrankshaft(CrankshaftNode *node) const {
    auto it = m_crankshafts.find(node);
    if (it != m_crankshafts.end()) {
        return it->second;
    }
    else return nullptr;
}

void es_script::EngineContext::addConnectingRod(
    ConnectingRodNode *node,
    ConnectingRod *rod)
{
    m_rods[node] = rod;
}

ConnectingRod *es_script::EngineContext::getConnectingRod(
    ConnectingRodNode *node) const
{
    auto it = m_rods.find(node);
    if (it != m_rods.end()) {
        return it->second;
    }
    else return nullptr;
}

void es_script::EngineContext::setCylinderIndex(
    const CylinderBankNode *bank,
    int localIndex,
    int globalIndex)
{
    m_cylinderIndices[{ bank, localIndex }] = globalIndex;
}

int es_script::EngineContext::getCylinderIndex(
    const CylinderBankNode *bank,
    int localIndex) const
{
    return m_cylinderIndices.at({ bank, localIndex });
}
