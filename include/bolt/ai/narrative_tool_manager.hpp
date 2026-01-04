#ifndef NARRATIVE_TOOL_MANAGER_HPP
#define NARRATIVE_TOOL_MANAGER_HPP

#include <string>
#include <vector>
#include <functional>
#include <unordered_map>
#include <memory>
#include <random>

namespace bolt {
namespace ai {

/**
 * Result of a tool invocation in the narrative context
 */
struct ToolResult {
    bool success;
    std::string description;    // Narrative description of the result
    std::string outcome;        // Structured outcome data
    int impact_score;           // How much this affects the world (1-10)
    std::vector<std::string> state_changes; // Changes to world state
};

/**
 * Definition of a narrative tool (character ability or world interaction)
 */
struct NarrativeTool {
    std::string name;
    std::string description;
    std::vector<std::string> parameters;
    std::string category;       // combat, social, exploration, magic, etc.
    int difficulty;             // Base difficulty (1-20)
    bool requires_target;
    std::function<ToolResult(const std::unordered_map<std::string, std::string>&)> handler;
};

/**
 * Character stats and abilities for tool use
 */
struct CharacterSheet {
    std::string name;
    std::unordered_map<std::string, int> skills; // skill_name -> proficiency (1-10)
    std::unordered_map<std::string, int> attributes; // strength, intelligence, etc.
    std::vector<std::string> known_abilities;
    int luck_modifier = 0;
};

/**
 * Manager for narrative tool-use in story generation
 * Enables character agency through skill-based tool invocations
 */
class NarrativeToolManager {
private:
    std::unordered_map<std::string, NarrativeTool> tools_;
    std::unordered_map<std::string, CharacterSheet> characters_;
    std::mt19937 rng_;
    bool initialized_ = false;
    
    // Game mechanics
    int rollDice(int sides, int count = 1);
    bool skillCheck(const std::string& character, 
                   const std::string& skill, 
                   int difficulty);
    
    // Tool execution
    ToolResult executeTool(const std::string& tool_name,
                          const std::string& character,
                          const std::unordered_map<std::string, std::string>& params);

public:
    NarrativeToolManager();
    ~NarrativeToolManager() = default;
    
    // Initialization
    bool initialize();
    bool isInitialized() const { return initialized_; }
    
    // Tool registration
    void registerTool(const NarrativeTool& tool);
    void unregisterTool(const std::string& tool_name);
    std::vector<std::string> getAvailableTools() const;
    NarrativeTool getTool(const std::string& tool_name) const;
    
    // Character management
    void registerCharacter(const CharacterSheet& character);
    void updateCharacterSkill(const std::string& character, 
                             const std::string& skill, 
                             int new_value);
    CharacterSheet getCharacter(const std::string& character) const;
    std::vector<std::string> getCharacterAbilities(const std::string& character) const;
    
    // Tool invocation
    ToolResult invokeTool(const std::string& tool_name,
                         const std::string& character,
                         const std::unordered_map<std::string, std::string>& params);
    
    // Tool discovery (for AI to query available actions)
    std::vector<std::string> getToolsForCharacter(const std::string& character) const;
    std::vector<std::string> getToolsByCategory(const std::string& category) const;
    std::string getToolsAsJSON() const;
    
    // Predefined tool sets
    void loadCombatTools();
    void loadSocialTools();
    void loadExplorationTools();
    void loadMagicTools();
    void loadAllStandardTools();
};

} // namespace ai
} // namespace bolt

#endif // NARRATIVE_TOOL_MANAGER_HPP
