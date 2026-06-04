#include "kita_pch.h"
#include "ShaderLabParser.h"

#include <cctype>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string_view>

namespace Kita {

	namespace {

		enum class TokenType
		{
			Identifier,
			String,
			Number,
			LBrace,
			RBrace,
			LParen,
			RParen,
			Comma,
			Equals,
			Semicolon,
			End
		};

		struct Token
		{
			TokenType Type = TokenType::End;
			std::string Text;
			int Line = 1;
			int Column = 1;
		};

		class ParseException : public std::runtime_error
		{
		public:
			ParseException(int line, int column, const std::string& message)
				: std::runtime_error(message)
				, Line(line)
				, Column(column)
			{
			}

			int Line = 0;
			int Column = 0;
		};

		static std::string ReadTextFile(const std::filesystem::path& path)
		{
			std::ifstream input(path, std::ios::binary);
			if (!input.is_open())
				return {};

			std::stringstream stream;
			stream << input.rdbuf();
			return stream.str();
		}

		static bool IsIdentifierStart(char c)
		{
			return std::isalpha(static_cast<unsigned char>(c)) != 0 || c == '_';
		}

		static bool IsIdentifierChar(char c)
		{
			return std::isalnum(static_cast<unsigned char>(c)) != 0 || c == '_';
		}

		static bool IsNumberStart(char c)
		{
			return std::isdigit(static_cast<unsigned char>(c)) != 0 || c == '-' || c == '+' || c == '.';
		}

		static std::string FormatErrorMessage(int line, int column, const std::string& message)
		{
			return "Line " + std::to_string(line) + ", Column " + std::to_string(column) + ": " + message;
		}

		class Lexer
		{
		public:
			explicit Lexer(std::string_view source)
				: m_Source(source)
			{
			}

			std::vector<Token> Tokenize()
			{
				std::vector<Token> tokens;
				while (!IsAtEnd())
				{
					SkipWhitespaceAndComments();

					if (IsAtEnd())
						break;

					const int tokenLine = m_Line;
					const int tokenColumn = m_Column;
					const char c = Peek();

					switch (c)
					{
					case '{': tokens.push_back(MakeSingleCharToken(TokenType::LBrace)); Advance(); break;
					case '}': tokens.push_back(MakeSingleCharToken(TokenType::RBrace)); Advance(); break;
					case '(': tokens.push_back(MakeSingleCharToken(TokenType::LParen)); Advance(); break;
					case ')': tokens.push_back(MakeSingleCharToken(TokenType::RParen)); Advance(); break;
					case ',': tokens.push_back(MakeSingleCharToken(TokenType::Comma)); Advance(); break;
					case '=': tokens.push_back(MakeSingleCharToken(TokenType::Equals)); Advance(); break;
					case ';': tokens.push_back(MakeSingleCharToken(TokenType::Semicolon)); Advance(); break;
					case '"': tokens.push_back(ReadStringToken()); break;
					default:
						if (IsIdentifierStart(c))
						{
							tokens.push_back(ReadIdentifierToken());
						}
						else if (IsNumberStart(c))
						{
							tokens.push_back(ReadNumberToken());
						}
						else
						{
							throw ParseException(
								tokenLine,
								tokenColumn,
								"Unexpected character '" + std::string(1, c) + "'");
						}
						break;
					}
				}

				tokens.push_back(Token{ TokenType::End, {}, m_Line, m_Column });
				return tokens;
			}

		private:
			bool IsAtEnd() const
			{
				return m_Index >= m_Source.size();
			}

			char Peek(size_t offset = 0) const
			{
				const size_t target = m_Index + offset;
				if (target >= m_Source.size())
					return '\0';
				return m_Source[target];
			}

			char Advance()
			{
				const char c = Peek();
				++m_Index;

				if (c == '\n')
				{
					++m_Line;
					m_Column = 1;
				}
				else
				{
					++m_Column;
				}

				return c;
			}

			void SkipWhitespaceAndComments()
			{
				while (!IsAtEnd())
				{
					const char c = Peek();

					if (std::isspace(static_cast<unsigned char>(c)) != 0)
					{
						Advance();
						continue;
					}

					// 单行注释
					if (c == '/' && Peek(1) == '/')
					{
						while (!IsAtEnd() && Peek() != '\n')
							Advance();
						continue;
					}

					// 多行注释
					if (c == '/' && Peek(1) == '*')
					{
						Advance();
						Advance();

						while (!IsAtEnd())
						{
							if (Peek() == '*' && Peek(1) == '/')
							{
								Advance();
								Advance();
								break;
							}
							Advance();
						}

						continue;
					}

					break;
				}
			}

			Token MakeSingleCharToken(TokenType type) const
			{
				return Token{ type, std::string(1, Peek()), m_Line, m_Column };
			}

			Token ReadIdentifierToken()
			{
				const int startLine = m_Line;
				const int startColumn = m_Column;
				std::string text;

				while (!IsAtEnd() && IsIdentifierChar(Peek()))
					text.push_back(Advance());

				return Token{ TokenType::Identifier, std::move(text), startLine, startColumn };
			}

			Token ReadNumberToken()
			{
				const int startLine = m_Line;
				const int startColumn = m_Column;
				std::string text;

				if (Peek() == '+' || Peek() == '-')
					text.push_back(Advance());

				bool hasDot = false;
				while (!IsAtEnd())
				{
					const char c = Peek();
					if (std::isdigit(static_cast<unsigned char>(c)) != 0)
					{
						text.push_back(Advance());
						continue;
					}

					if (c == '.' && !hasDot)
					{
						hasDot = true;
						text.push_back(Advance());
						continue;
					}

					break;
				}

				return Token{ TokenType::Number, std::move(text), startLine, startColumn };
			}

			Token ReadStringToken()
			{
				const int startLine = m_Line;
				const int startColumn = m_Column;
				std::string text;

				Advance(); // skip opening quote

				while (!IsAtEnd())
				{
					const char c = Advance();
					if (c == '"')
					{
						return Token{ TokenType::String, std::move(text), startLine, startColumn };
					}

					if (c == '\\')
					{
						if (IsAtEnd())
							break;

						const char escaped = Advance();
						switch (escaped)
						{
						case '"': text.push_back('"'); break;
						case '\\': text.push_back('\\'); break;
						case 'n': text.push_back('\n'); break;
						case 't': text.push_back('\t'); break;
						default: text.push_back(escaped); break;
						}
						continue;
					}

					text.push_back(c);
				}

				throw ParseException(startLine, startColumn, "Unterminated string literal");
			}

		private:
			std::string_view m_Source;
			size_t m_Index = 0;
			int m_Line = 1;
			int m_Column = 1;
		};

		static bool TryParseBool(const std::string& text, bool& outValue)
		{
			if (text == "True" || text == "true" || text == "On")
			{
				outValue = true;
				return true;
			}

			if (text == "False" || text == "false" || text == "Off")
			{
				outValue = false;
				return true;
			}

			return false;
		}

		static float ParseFloatChecked(const Token& token)
		{
			try
			{
				return std::stof(token.Text);
			}
			catch (...)
			{
				throw ParseException(token.Line, token.Column, "Invalid float literal '" + token.Text + "'");
			}
		}

		static int32_t ParseIntChecked(const Token& token)
		{
			try
			{
				return std::stoi(token.Text);
			}
			catch (...)
			{
				throw ParseException(token.Line, token.Column, "Invalid int literal '" + token.Text + "'");
			}
		}

		static uint32_t ParseUIntChecked(const Token& token)
		{
			const int32_t value = ParseIntChecked(token);
			if (value < 0)
			{
				throw ParseException(token.Line, token.Column, "Expected unsigned integer literal '" + token.Text + "'");
			}

			return static_cast<uint32_t>(value);
		}

		static bool TryParsePropertyType(const std::string& text, MaterialValueType& outType)
		{
			if (text == "Bool") { outType = MaterialValueType::Bool; return true; }
			if (text == "Int") { outType = MaterialValueType::Int; return true; }
			if (text == "Float") { outType = MaterialValueType::Float; return true; }
			if (text == "Float2") { outType = MaterialValueType::Float2; return true; }
			if (text == "Float3") { outType = MaterialValueType::Float3; return true; }
			if (text == "Float4") { outType = MaterialValueType::Float4; return true; }
			if (text == "Vector") { outType = MaterialValueType::Float4; return true; }
			if (text == "Color") { outType = MaterialValueType::Color; return true; }
			if (text == "Texture2D") { outType = MaterialValueType::Texture2D; return true; }
			if (text == "TextureCube") { outType = MaterialValueType::TextureCube; return true; }
			return false;
		}

		static PassType ParsePassType(const std::string& lightMode)
		{
			if (lightMode == "GBuffer") return PassType::GBuffer;
			if (lightMode == "DeferredLighting") return PassType::DeferredLighting;
			if (lightMode == "ShadowCaster") return PassType::ShadowCaster;
			if (lightMode == "DepthOnly") return PassType::DepthOnly;
			if (lightMode == "ForwardOpaque") return PassType::ForwardOpaque;
			if (lightMode == "ForwardTransparent") return PassType::ForwardTransparent;
			if (lightMode == "PostProcess") return PassType::PostProcess;
			return PassType::Unknown;
		}

		static VkCullModeFlags ParseCullMode(const Token& token)
		{
			if (token.Text == "Off") return VK_CULL_MODE_NONE;
			if (token.Text == "Back") return VK_CULL_MODE_BACK_BIT;
			if (token.Text == "Front") return VK_CULL_MODE_FRONT_BIT;
			throw ParseException(token.Line, token.Column, "Unsupported Cull mode '" + token.Text + "'");
		}

		static VkCompareOp ParseCompareOp(const Token& token)
		{
			if (token.Text == "Less") return VK_COMPARE_OP_LESS;
			if (token.Text == "LessEqual") return VK_COMPARE_OP_LESS_OR_EQUAL;
			if (token.Text == "Greater") return VK_COMPARE_OP_GREATER;
			if (token.Text == "Always") return VK_COMPARE_OP_ALWAYS;
			if (token.Text == "Equal") return VK_COMPARE_OP_EQUAL;
			throw ParseException(token.Line, token.Column, "Unsupported compare op '" + token.Text + "'");
		}

		static VkFormat ParseFormat(const Token& token)
		{
			if (token.Text == "R8G8B8A8_UNORM") return VK_FORMAT_R8G8B8A8_UNORM;
			if (token.Text == "R8G8B8A8_SRGB") return VK_FORMAT_R8G8B8A8_SRGB;
			if (token.Text == "R16G16B16A16_SFLOAT") return VK_FORMAT_R16G16B16A16_SFLOAT;
			if (token.Text == "D32_SFLOAT") return VK_FORMAT_D32_SFLOAT;
			throw ParseException(token.Line, token.Column, "Unsupported VkFormat '" + token.Text + "'");
		}

		class Parser
		{
		public:
			Parser(std::vector<Token> tokens, const std::filesystem::path& sourcePath)
				: m_Tokens(std::move(tokens))
				, m_SourcePath(sourcePath)
			{
			}

			ShaderLabAssetDesc Parse()
			{
				ShaderLabAssetDesc desc{};

				ConsumeIdentifierText("Shader");
				desc.ShaderName = Consume(TokenType::String, "shader name").Text;
				Consume(TokenType::LBrace, "'{' after shader name");

				while (!Check(TokenType::RBrace))
				{
					SkipOptionalSemicolons();

					if (MatchIdentifierText("Properties"))
					{
						ParsePropertiesBlock(desc);
						continue;
					}

					if (MatchIdentifierText("Tags"))
					{
						ParseTagsBlock(desc.Tags);
						continue;
					}

					if (MatchIdentifierText("Lighting"))
					{
						ParseLightingBlock(desc.Lighting);
						continue;
					}

					if (MatchIdentifierText("Pass"))
					{
						desc.Passes.push_back(ParsePassBlock());
						continue;
					}

					const Token& token = Peek();
					throw ParseException(token.Line, token.Column, "Unexpected top-level token '" + token.Text + "'");
				}

				Consume(TokenType::RBrace, "'}' at end of shader");

				if (desc.ShaderName.empty())
				{
					const Token& token = Peek();
					throw ParseException(token.Line, token.Column, "Shader name cannot be empty");
				}

				if (desc.Passes.empty())
				{
					const Token& token = Peek();
					throw ParseException(token.Line, token.Column, "ShaderLab asset must contain at least one Pass");
				}

				return desc;
			}

		private:
			const Token& Peek(size_t offset = 0) const
			{
				const size_t index = std::min(m_Index + offset, m_Tokens.size() - 1);
				return m_Tokens[index];
			}

			bool Check(TokenType type) const
			{
				return Peek().Type == type;
			}

			bool CheckIdentifierText(const char* text) const
			{
				return Peek().Type == TokenType::Identifier && Peek().Text == text;
			}

			bool Match(TokenType type)
			{
				if (!Check(type))
					return false;
				++m_Index;
				return true;
			}

			bool MatchIdentifierText(const char* text)
			{
				if (!CheckIdentifierText(text))
					return false;
				++m_Index;
				return true;
			}

			const Token& Consume(TokenType type, const std::string& what)
			{
				if (!Check(type))
				{
					const Token& token = Peek();
					throw ParseException(token.Line, token.Column, "Expected " + what);
				}

				return m_Tokens[m_Index++];
			}

			const Token& ConsumeIdentifier(const std::string& what)
			{
				return Consume(TokenType::Identifier, what);
			}

			void ConsumeIdentifierText(const char* text)
			{
				const Token& token = ConsumeIdentifier(std::string("keyword '") + text + "'");
				if (token.Text != text)
				{
					throw ParseException(token.Line, token.Column, "Expected keyword '" + std::string(text) + "'");
				}
			}

			void SkipOptionalSemicolons()
			{
				while (Match(TokenType::Semicolon))
				{
				}
			}

			std::string ParseLooseValueText()
			{
				if (Check(TokenType::Identifier) || Check(TokenType::String) || Check(TokenType::Number))
				{
					return m_Tokens[m_Index++].Text;
				}

				const Token& token = Peek();
				throw ParseException(token.Line, token.Column, "Expected identifier, string or number value");
			}

			std::vector<float> ParseFloatTuple()
			{
				std::vector<float> values;
				Consume(TokenType::LParen, "'('");

				values.push_back(ParseFloatChecked(Consume(TokenType::Number, "number")));
				while (Match(TokenType::Comma))
				{
					values.push_back(ParseFloatChecked(Consume(TokenType::Number, "number")));
				}

				Consume(TokenType::RParen, "')'");
				return values;
			}

			MaterialPropertyValue ParseDefaultValue(MaterialValueType type)
			{
				MaterialPropertyValue value{};
				value.ValueType = type;

				switch (type)
				{
				case MaterialValueType::Bool:
				{
					const std::string text = ParseLooseValueText();
					bool boolValue = false;
					if (!TryParseBool(text, boolValue))
					{
						const Token& token = Peek();
						throw ParseException(token.Line, token.Column, "Invalid bool literal '" + text + "'");
					}
					value.Data = boolValue;
					return value;
				}
				case MaterialValueType::Int:
				{
					value.Data = ParseIntChecked(Consume(TokenType::Number, "integer"));
					return value;
				}
				case MaterialValueType::Float:
				{
					value.Data = ParseFloatChecked(Consume(TokenType::Number, "float"));
					return value;
				}
				case MaterialValueType::Float2:
				{
					const std::vector<float> tuple = ParseFloatTuple();
					if (tuple.size() != 2)
					{
						const Token& token = Peek();
						throw ParseException(token.Line, token.Column, "Float2 default value must contain 2 numbers");
					}
					value.Data = glm::vec2(tuple[0], tuple[1]);
					return value;
				}
				case MaterialValueType::Float3:
				{
					const std::vector<float> tuple = ParseFloatTuple();
					if (tuple.size() != 3)
					{
						const Token& token = Peek();
						throw ParseException(token.Line, token.Column, "Float3 default value must contain 3 numbers");
					}
					value.Data = glm::vec3(tuple[0], tuple[1], tuple[2]);
					return value;
				}
				case MaterialValueType::Float4:
				case MaterialValueType::Color:
				{
					const std::vector<float> tuple = ParseFloatTuple();
					if (tuple.size() != 4)
					{
						const Token& token = Peek();
						throw ParseException(token.Line, token.Column, "Float4/Color default value must contain 4 numbers");
					}
					value.Data = glm::vec4(tuple[0], tuple[1], tuple[2], tuple[3]);
					return value;
				}
				case MaterialValueType::Texture2D:
				case MaterialValueType::TextureCube:
				{
					value.Data = Consume(TokenType::String, "texture default string").Text;
					return value;
				}
				default:
					break;
				}

				const Token& token = Peek();
				throw ParseException(token.Line, token.Column, "Unsupported property default value type");
			}

			void ParsePropertiesBlock(ShaderLabAssetDesc& outDesc)
			{
				Consume(TokenType::LBrace, "'{' after Properties");

				while (!Check(TokenType::RBrace))
				{
					SkipOptionalSemicolons();
					if (Check(TokenType::RBrace))
						break;

					MaterialPropertyDesc property{};
					property.Name = ConsumeIdentifier("property name").Text;

					Consume(TokenType::LParen, "'(' after property name");
					property.DisplayName = Consume(TokenType::String, "property display name").Text;
					Consume(TokenType::Comma, "',' after property display name");

					if (MatchIdentifierText("Range"))
					{
						property.ValueType = MaterialValueType::Float;
						property.UIHint.HasRange = true;

						Consume(TokenType::LParen, "'(' after Range");
						property.UIHint.MinValue = ParseFloatChecked(Consume(TokenType::Number, "range minimum"));
						Consume(TokenType::Comma, "',' in Range");
						property.UIHint.MaxValue = ParseFloatChecked(Consume(TokenType::Number, "range maximum"));
						Consume(TokenType::RParen, "')' after Range");
					}
					else
					{
						const Token& typeToken = ConsumeIdentifier("property type");
						if (!TryParsePropertyType(typeToken.Text, property.ValueType))
						{
							throw ParseException(typeToken.Line, typeToken.Column, "Unsupported property type '" + typeToken.Text + "'");
						}
					}

					Consume(TokenType::RParen, "')' after property type");
					Consume(TokenType::Equals, "'=' after property declaration");
					property.DefaultValue = ParseDefaultValue(property.ValueType);

					outDesc.Properties.push_back(std::move(property));
					SkipOptionalSemicolons();
				}

				Consume(TokenType::RBrace, "'}' after Properties");
			}

			void ParseTagsBlock(std::unordered_map<std::string, std::string>& outTags)
			{
				Consume(TokenType::LBrace, "'{' after Tags");

				while (!Check(TokenType::RBrace))
				{
					SkipOptionalSemicolons();
					if (Check(TokenType::RBrace))
						break;

					const std::string key = ConsumeIdentifier("tag key").Text;
					Consume(TokenType::Equals, "'=' after tag key");
					const std::string value = ParseLooseValueText();
					outTags[key] = value;

					SkipOptionalSemicolons();
				}

				Consume(TokenType::RBrace, "'}' after Tags");
			}

			void ParseLightingBlock(ShaderLabLightingDesc& outLighting)
			{
				outLighting.Enabled = true;
				Consume(TokenType::LBrace, "'{' after Lighting");

				while (!Check(TokenType::RBrace))
				{
					SkipOptionalSemicolons();
					if (Check(TokenType::RBrace))
						break;

					const Token& keyToken = ConsumeIdentifier("Lighting key");
					Consume(TokenType::Equals, "'=' after Lighting key");

					if (keyToken.Text == "ShadingModel")
					{
						outLighting.ShadingModel = ParseLooseValueText();
					}
					else if (keyToken.Text == "CustomDataCount")
					{
						outLighting.CustomDataCount = ParseUIntChecked(Consume(TokenType::Number, "custom data count"));
					}
					else if (keyToken.Text == "Include")
					{
						const std::string relativePath = Consume(TokenType::String, "lighting hook include path").Text;
						outLighting.Include = m_SourcePath.empty()
							? std::filesystem::path(relativePath)
							: (m_SourcePath.parent_path() / relativePath).lexically_normal();
					}
					else if (keyToken.Text == "Evaluate")
					{
						outLighting.Evaluate = ParseLooseValueText();
					}
					else
					{
						throw ParseException(keyToken.Line, keyToken.Column, "Unsupported Lighting key '" + keyToken.Text + "'");
					}

					SkipOptionalSemicolons();
				}

				Consume(TokenType::RBrace, "'}' after Lighting");
			}

			void ApplyRenderStateAssignment(ShaderLabRenderStateDesc& state, const Token& keyToken, const Token& valueToken)
			{
				if (keyToken.Text == "Cull")
				{
					state.CullMode = ParseCullMode(valueToken);
					return;
				}

				if (keyToken.Text == "DepthWrite")
				{
					bool boolValue = false;
					if (!TryParseBool(valueToken.Text, boolValue))
					{
						throw ParseException(valueToken.Line, valueToken.Column, "DepthWrite only accepts On/Off or True/False");
					}
					state.DepthWrite = boolValue;
					return;
				}

				if (keyToken.Text == "Blend")
				{
					bool boolValue = false;
					if (!TryParseBool(valueToken.Text, boolValue))
					{
						throw ParseException(valueToken.Line, valueToken.Column, "Blend only accepts On/Off or True/False");
					}
					state.Blend = boolValue;
					return;
				}

				if (keyToken.Text == "DepthTest")
				{
					bool boolValue = false;
					if (TryParseBool(valueToken.Text, boolValue))
					{
						state.DepthTest = boolValue;
						return;
					}

					state.DepthTest = true;
					state.DepthCompareOp = ParseCompareOp(valueToken);
					return;
				}

				throw ParseException(keyToken.Line, keyToken.Column, "Unsupported RenderState key '" + keyToken.Text + "'");
			}

			void ParseRenderStateBlock(ShaderLabRenderStateDesc& outState)
			{
				Consume(TokenType::LBrace, "'{' after RenderState");

				while (!Check(TokenType::RBrace))
				{
					SkipOptionalSemicolons();
					if (Check(TokenType::RBrace))
						break;

					const Token& keyToken = ConsumeIdentifier("RenderState key");
					Consume(TokenType::Equals, "'=' after RenderState key");
					const Token& valueToken = m_Tokens[m_Index++];
					if (valueToken.Type != TokenType::Identifier && valueToken.Type != TokenType::String)
					{
						throw ParseException(valueToken.Line, valueToken.Column, "RenderState value must be identifier or string");
					}

					ApplyRenderStateAssignment(outState, keyToken, valueToken);
					SkipOptionalSemicolons();
				}

				Consume(TokenType::RBrace, "'}' after RenderState");
			}

			void ParseProgramBlock(ShaderLabProgramDesc& outProgram)
			{
				Consume(TokenType::LBrace, "'{' after Program");

				while (!Check(TokenType::RBrace))
				{
					SkipOptionalSemicolons();
					if (Check(TokenType::RBrace))
						break;

					const Token& keyToken = ConsumeIdentifier("Program key");
					Consume(TokenType::Equals, "'=' after Program key");

					if (keyToken.Text == "Source")
					{
						const std::string relativePath = Consume(TokenType::String, "program source path").Text;
						outProgram.Source = m_SourcePath.empty()
							? std::filesystem::path(relativePath)
							: (m_SourcePath.parent_path() / relativePath).lexically_normal();
					}
					else if (keyToken.Text == "Vertex")
					{
						outProgram.VertexEntry = ParseLooseValueText();
					}
					else if (keyToken.Text == "Fragment")
					{
						outProgram.FragmentEntry = ParseLooseValueText();
					}
					else
					{
						throw ParseException(keyToken.Line, keyToken.Column, "Unsupported Program key '" + keyToken.Text + "'");
					}

					SkipOptionalSemicolons();
				}

				Consume(TokenType::RBrace, "'}' after Program");
			}

			void ParseRenderGraphBlock(ShaderLabRenderGraphDesc& outGraph)
			{
				Consume(TokenType::LBrace, "'{' after RenderGraph");

				while (!Check(TokenType::RBrace))
				{
					SkipOptionalSemicolons();
					if (Check(TokenType::RBrace))
						break;

					const Token& keyToken = ConsumeIdentifier("RenderGraph key");

					if (keyToken.Text == "Color")
					{
						const std::string colorName = Consume(TokenType::String, "color output name").Text;
						Consume(TokenType::Equals, "'=' after color output name");
						const Token& formatToken = ConsumeIdentifier("color format");

						ShaderLabColorOutputDesc color{};
						color.Name = colorName;
						color.Format = ParseFormat(formatToken);
						outGraph.Colors.push_back(std::move(color));
						SkipOptionalSemicolons();
						continue;
					}

					if (keyToken.Text == "Depth")
					{
						const std::string depthName = Consume(TokenType::String, "depth output name").Text;
						Consume(TokenType::Equals, "'=' after depth output name");
						const Token& formatToken = ConsumeIdentifier("depth format");

						outGraph.HasDepth = true;
						outGraph.DepthName = depthName;
						outGraph.DepthFormat = ParseFormat(formatToken);
						SkipOptionalSemicolons();
						continue;
					}

					Consume(TokenType::Equals, "'=' after RenderGraph key");
					const std::string value = ParseLooseValueText();

					if (keyToken.Text == "InputLayout")
					{
						outGraph.InputLayout = value;
					}
					else if (keyToken.Text == "OutputLayout")
					{
						outGraph.OutputLayout = value;
					}
					else
					{
						throw ParseException(keyToken.Line, keyToken.Column, "Unsupported RenderGraph key '" + keyToken.Text + "'");
					}

					SkipOptionalSemicolons();
				}

				Consume(TokenType::RBrace, "'}' after RenderGraph");
			}

			ShaderLabPassDesc ParsePassBlock()
			{
				ShaderLabPassDesc pass{};
				pass.Name = Consume(TokenType::String, "pass name").Text;
				Consume(TokenType::LBrace, "'{' after pass name");

				while (!Check(TokenType::RBrace))
				{
					SkipOptionalSemicolons();
					if (Check(TokenType::RBrace))
						break;

					const Token& keyToken = ConsumeIdentifier("pass member");

					if (keyToken.Text == "Tags")
					{
						ParseTagsBlock(pass.Tags);
						continue;
					}

					if (keyToken.Text == "RenderState")
					{
						ParseRenderStateBlock(pass.RenderState);
						continue;
					}

					if (keyToken.Text == "RenderGraph")
					{
						ParseRenderGraphBlock(pass.RenderGraph);
						continue;
					}

					if (keyToken.Text == "Program")
					{
						ParseProgramBlock(pass.Program);
						continue;
					}

					Consume(TokenType::Equals, "'=' after pass member");

					if (keyToken.Text == "LightMode")
					{
						pass.LightMode = ParseLooseValueText();
						pass.Type = ParsePassType(pass.LightMode);
						SkipOptionalSemicolons();
						continue;
					}

					// 兼容把 RenderState 直接平铺在 Pass 内部的写法
					if (keyToken.Text == "Cull" || keyToken.Text == "DepthTest" || keyToken.Text == "DepthWrite" || keyToken.Text == "Blend")
					{
						const Token& valueToken = m_Tokens[m_Index++];
						if (valueToken.Type != TokenType::Identifier && valueToken.Type != TokenType::String)
						{
							throw ParseException(valueToken.Line, valueToken.Column, "RenderState assignment must be identifier or string");
						}

						ApplyRenderStateAssignment(pass.RenderState, keyToken, valueToken);
						SkipOptionalSemicolons();
						continue;
					}

					// 兼容把 Program 直接平铺在 Pass 内部的写法
					if (keyToken.Text == "Source")
					{
						const std::string relativePath = Consume(TokenType::String, "program source path").Text;
						pass.Program.Source = m_SourcePath.empty()
							? std::filesystem::path(relativePath)
							: (m_SourcePath.parent_path() / relativePath).lexically_normal();
						SkipOptionalSemicolons();
						continue;
					}

					if (keyToken.Text == "Vertex")
					{
						pass.Program.VertexEntry = ParseLooseValueText();
						SkipOptionalSemicolons();
						continue;
					}

					if (keyToken.Text == "Fragment")
					{
						pass.Program.FragmentEntry = ParseLooseValueText();
						SkipOptionalSemicolons();
						continue;
					}

					throw ParseException(keyToken.Line, keyToken.Column, "Unsupported pass member '" + keyToken.Text + "'");
				}

				Consume(TokenType::RBrace, "'}' after Pass");

				if (pass.LightMode.empty())
				{
					pass.LightMode = pass.Name;
					pass.Type = ParsePassType(pass.LightMode);
				}

				if (pass.Program.Source.empty())
				{
					const Token& token = Peek();
					throw ParseException(token.Line, token.Column, "Pass '" + pass.Name + "' missing Program.Source");
				}

				return pass;
			}

		private:
			std::vector<Token> m_Tokens;
			size_t m_Index = 0;
			std::filesystem::path m_SourcePath;
		};

	}

	ShaderLabParseResult ShaderLabParser::ParseFile(const std::filesystem::path& path)
	{
		const std::string source = ReadTextFile(path);
		if (source.empty())
		{
			ShaderLabParseResult result{};
			result.Success = false;
			result.Error.Message = "ShaderLab file is empty or cannot be opened";
			return result;
		}

		return ParseText(source, path);
	}

	ShaderLabParseResult ShaderLabParser::ParseText(std::string_view source, const std::filesystem::path& sourcePath)
	{
		ShaderLabParseResult result{};

		try
		{
			Lexer lexer(source);
			std::vector<Token> tokens = lexer.Tokenize();

			Parser parser(std::move(tokens), sourcePath);
			result.Asset = parser.Parse();
			result.Success = true;
		}
		catch (const ParseException& e)
		{
			result.Success = false;
			result.Error.Line = e.Line;
			result.Error.Column = e.Column;
			result.Error.Message = FormatErrorMessage(e.Line, e.Column, e.what());
		}
		catch (const std::exception& e)
		{
			result.Success = false;
			result.Error.Message = e.what();
		}

		return result;
	}

}
