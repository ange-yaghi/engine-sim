#include "../include/compiler.h"

es_script::Compiler::Output *es_script::Compiler::s_output = nullptr;

es_script::Compiler::Compiler() {
    m_compiler = nullptr;
}

es_script::Compiler::~Compiler() {
    assert(m_compiler == nullptr);
}

es_script::Compiler::Output *es_script::Compiler::output() {
    if (s_output == nullptr) {
        s_output = new Output;
    }

    return s_output;
}

void es_script::Compiler::initialize() {
    m_compiler = new piranha::Compiler(&m_rules);
    m_compiler->setFileExtension(".mr");

    m_compiler->addSearchPath("../../es/");
    m_compiler->addSearchPath("../es/");
    m_compiler->addSearchPath("es/");

    m_rules.initialize();
}

void es_script::Compiler::compile(const piranha::IrPath &path) {
    piranha::IrCompilationUnit *unit = m_compiler->compile(path);
    if (unit == nullptr) {
        // Todo: Could not find file
    }
    else {
        const piranha::ErrorList *errors = m_compiler->getErrorList();
        if (errors->getErrorCount() == 0) {
            unit->build(&m_program);

            m_program.initialize();
        }
        else {
            // Todo: Compilation errors
        }
    }
}

es_script::Compiler::Output es_script::Compiler::execute() {
    const bool result = m_program.execute();

    if (!result) {
        // Todo: Runtime error
    }

    return *s_output;
}

void es_script::Compiler::destroy() {
    m_program.free();
    m_compiler->free();

    delete m_compiler;
    m_compiler = nullptr;
}
