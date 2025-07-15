static bool isHexDigit(char c) {
    return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
}

static std::string unescapeRustSymbol(const std::string &input) {
    const char* rest = input.c_str();
    size_t len = input.length();
    std::string result;
    
    while (len > 0) {
        if (rest[0] == '.') {
            if (len >= 2 && rest[1] == '.') {
                result += "::";
                rest += 2;
                len -= 2;
            } else {
                result += ".";
                rest += 1;
                len -= 1;
            }
        } else if (rest[0] == '$') {
            const char *escape = (const char*)memchr(rest + 1, '$', len - 1);
            if (escape == nullptr) {
                result += rest[0];
                rest += 1;
                len -= 1;
                continue;
            }
            
            const char *escape_start = rest + 1;
            size_t escape_len = escape - (rest + 1);
            size_t next_len = len - (escape + 1 - rest);
            const char *next_rest = escape + 1;
            
            char ch = '\0';
            bool found = false;
            
            if (escape_len == 2 && escape_start[0] == 'S' && escape_start[1] == 'P') {
                ch = '@'; found = true;
            } else if (escape_len == 2 && escape_start[0] == 'B' && escape_start[1] == 'P') {
                ch = '*'; found = true;
            } else if (escape_len == 2 && escape_start[0] == 'R' && escape_start[1] == 'F') {
                ch = '&'; found = true;
            } else if (escape_len == 2 && escape_start[0] == 'L' && escape_start[1] == 'T') {
                ch = '<'; found = true;
            } else if (escape_len == 2 && escape_start[0] == 'G' && escape_start[1] == 'T') {
                ch = '>'; found = true;
            } else if (escape_len == 2 && escape_start[0] == 'L' && escape_start[1] == 'P') {
                ch = '('; found = true;
            } else if (escape_len == 2 && escape_start[0] == 'R' && escape_start[1] == 'P') {
                ch = ')'; found = true;
            } else if (escape_len == 1 && escape_start[0] == 'C') {
                ch = ','; found = true;
            } else if (escape_len > 1 && escape_start[0] == 'u') {
                std::string hex_str(escape_start + 1, escape_len - 1);
                char *end;
                unsigned long val = strtoul(hex_str.c_str(), &end, 16);
                if (*end == '\0' && val <= 127) {
                    ch = (char)val;
                    found = true;
                }
            }
            
            if (found) {
                result += ch;
                len = next_len;
                rest = next_rest;
            } else {
                result += rest[0];
                rest += 1;
                len -= 1;
            }
        } else {
            size_t j = 0;
            for (; j < len && rest[j] != '$' && rest[j] != '.'; j++);
            if (j == len) {
                result.append(rest, len);
                break;
            }
            result.append(rest, j);
            rest += j;
            len -= j;
        }
    }
    
    return result;
}

static std::string stripSymbolPrefix(const std::string &sym) {
    if (sym.length() >= 4 && sym.substr(0, 4) == "__ZN") {
        return sym.substr(4);
    }
    if (sym.length() >= 3 && sym.substr(0, 3) == "_ZN") {
        return sym.substr(3);
    }
    if (sym.length() >= 2 && sym.substr(0, 2) == "ZN") {
        return sym.substr(2);
    }
    
    // Handle v0 mangling
    if (sym.length() >= 2 && sym.substr(0, 2) == "_R") {
        return sym.substr(2);
    }
    if (sym.length() >= 1 && sym.substr(0, 1) == "R") {
        return sym.substr(1);
    }
    if (sym.length() >= 3 && sym.substr(0, 3) == "__R") {
        return sym.substr(3);
    }
    
    return "";
}

static std::vector<std::string> splitSymbolIntoElementsLegacy(const std::string &legacySymbol) {
    if (legacySymbol.empty()) {
        return {};
    }
    
    // Find the last 'h' which should be the start of the hash
    size_t hash_pos = legacySymbol.rfind('h');
    if (hash_pos == std::string::npos || hash_pos + 16 >= legacySymbol.length()) {
        return {};
    }
    
    // Verify that the 16 characters after 'h' are hex digits
    for (size_t i = hash_pos + 1; i < hash_pos + 17; i++) {
        if (i >= legacySymbol.length() || !isHexDigit(legacySymbol[i])) {
            return {};
        }
    }
    
    // Check that we're at the end (hash should be the last thing)
    if (hash_pos + 17 != legacySymbol.length()) {
        return {};
    }
    
    // Work backwards from hash_pos to find the length digits for the hash element
    size_t hash_len_end = hash_pos;
    size_t hash_len_start = hash_len_end;
    
    // Find the start of the length digits (should be "17" for h + 16 hex chars)
    while (hash_len_start > 0 && std::isdigit(legacySymbol[hash_len_start - 1])) {
        hash_len_start--;
    }
    
    if (hash_len_start >= hash_len_end) {
        return {};
    }
    
    // Parse the length - avoid try-catch since exceptions are disabled
    std::string len_str = legacySymbol.substr(hash_len_start, hash_len_end - hash_len_start);
    char *end;
    long hash_element_len = strtol(len_str.c_str(), &end, 10);
    if (*end != '\0' || hash_element_len < 0) {
        return {};
    }
    
    // Verify the length matches (should be 17 for "h" + 16 hex digits)
    if (hash_element_len != 17) {
        return {};
    }
    
    // Now parse the elements before the hash
    size_t elements_end = hash_len_start;
    size_t cursor = 0;
    size_t idx = 0;
    std::vector<std::string> legacySymbolElements;
    
    while (idx < elements_end) {
        char c = legacySymbol[idx];
        if (std::isdigit(c)) {
            cursor = cursor * 10 + (c - '0');
            idx++;
        } else {
            if (cursor == 0) {
                return {};
            }
            
            if (idx + cursor > elements_end) {
                return {};
            }
            
            legacySymbolElements.push_back(legacySymbol.substr(idx, cursor));
            idx += cursor;
            cursor = 0;
        }
    }
    
    // Make sure we consumed all elements
    if (cursor != 0) {
        return {};
    }
    
    return legacySymbolElements;
}

static std::string demangleSymbolLegacy(const std::string &symbol) {
    if (symbol.empty()) {
        return symbol;
    }
    
    // Must end with 'E' for legacy format
    if (symbol.back() != 'E') {
        // Try to unescape anyway in case it's partial
        return unescapeRustSymbol(symbol);
    }
    
    // Remove the 'E' suffix
    std::string without_e = symbol.substr(0, symbol.length() - 1);
    
    std::string legacySymbolStripped = stripSymbolPrefix(without_e);
    if (legacySymbolStripped.empty()) {
        return unescapeRustSymbol(symbol);
    }
    
    // Try to parse as traditional length-prefixed legacy format
    std::vector<std::string> legacySymbolElements = splitSymbolIntoElementsLegacy(legacySymbolStripped);
    if (!legacySymbolElements.empty()) {
        std::vector<std::string> legacyElementsDemangled;
        for (const std::string &element : legacySymbolElements) {
            legacyElementsDemangled.push_back(unescapeRustSymbol(element));
        }
        
        std::string result;
        for (size_t idx = 0; idx < legacyElementsDemangled.size(); idx++) {
            if (idx > 0)
                result += "::";
            result += legacyElementsDemangled[idx];
        }
        return result;
    } else {
        return unescapeRustSymbol(symbol);
    }
}

static std::string rustDemangle(const std::string &symbol) {
    // Handle LLVM suffixes first
    std::string s = symbol;
    size_t llvm_pos = s.find(".llvm.");
    if (llvm_pos != std::string::npos) {
        bool all_hex = true;
        for (size_t i = llvm_pos + 6; i < s.length(); i++) {
            char c = s[i];
            if (!((c >= '0' && c <= '9') || (c >= 'A' && c <= 'F') || c == '@')) {
                all_hex = false;
                break;
            }
        }
        if (all_hex) {
            s = s.substr(0, llvm_pos);
        }
    }
    
    return demangleSymbolLegacy(s);
}