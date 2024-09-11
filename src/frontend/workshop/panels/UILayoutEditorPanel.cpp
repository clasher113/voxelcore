#include "../../../coders/xml.hpp"
#include "../../../content/Content.hpp"
#include "../../../files/files.hpp"
#include "../../../window/Events.hpp"
#include "../IncludeCommons.hpp"
#include "../WorkshopPreview.hpp"
#include "../WorkshopScreen.hpp"
#include "../WorkshopSerializer.hpp"

void workshop::WorkShopScreen::createUILayoutEditor(const fs::path& path, const std::string& fullName, const std::string& actualName,
	std::vector<size_t> docPath)
{
	createPanel([this, path, fullName, actualName, docPath]() {
		gui::Panel& panel = *new gui::Panel(glm::vec2(250));

		if (xmlDocs.find(fullName) == xmlDocs.end()) {
			xmlDocs.emplace(fullName, xml::parse(path.u8string(), files::read_string(path)));
		}

		std::shared_ptr<xml::Document> xmlDoc(xmlDocs[fullName]);
		auto updatePreview = [this, xmlDoc](bool forceUpdate = true) {
			preview->setUiDocument(xmlDoc, engine->getContent()->getPackRuntime(currentPackId)->getEnvironment(), forceUpdate);
		};
		updatePreview(false);

		panel += new gui::Label(actualName);
		panel += new gui::Label("Root type: " + xmlDoc->getRoot()->getTag());
		panel += new gui::Label("Current path:");

		auto getElement = [xmlDoc](const std::vector<size_t>& path) {
			xml::xmlelement result = xmlDoc->getRoot();
			for (const size_t elem : path) {
				result = result->getElements()[elem];
			}
			return result;
		};

		std::string pathString("[root]/");
		for (size_t i = 1; i <= docPath.size(); i++) {
			pathString.append(getElement(std::vector<size_t>(docPath.begin(), docPath.begin() + i))->getTag() + '/');
		}
		panel += new gui::Label(pathString);

		xml::xmlelement currentElement = getElement(docPath);
		xml::xmlelement previousElement;
		const bool root = docPath.empty();

		if (!root) {
			previousElement = getElement(std::vector<size_t>(docPath.begin(), docPath.end() - 1));
		}

		const unsigned long long currentTag = uiElementsArgs.at(currentElement->getTag()).args;
		const unsigned long long previousTag = (root ? 0 : uiElementsArgs.at(getElement(std::vector<size_t>(docPath.begin(), docPath.end() - 1))->getTag()).args);
		const std::vector<std::string> listOfHorizontalAlignments{ "left", "center", "right" };
		const std::vector<std::string> listOfVerticalAlignments{ "top", "center", "bottom" };
		const std::vector<std::string> listOfOrientations{ "horizontal" };
		const std::vector<std::string> listOfGravity{ "top-left", "top-center", "top-right", "center-left", "center-center", "center-right", "bottom-left",
			"bottom-center", "bottom-right"
		};

		auto goTo = [this, path, fullName, actualName](const std::vector<size_t> docPath) {
			createUILayoutEditor(path, fullName, actualName, docPath);
		};

		if (!root) {
			panel += new gui::Button(L"Back", glm::vec4(10.f), [dp = docPath, goTo](gui::GUI*) mutable {
				dp.pop_back();
				goTo(dp);
			});
		}
		panel += new gui::Label("Current element: " + currentElement->getTag());
		if (currentTag & INVENTORY || currentTag & CONTAINER || currentTag & PANEL) {
			panel += new gui::Button(L"Add element", glm::vec4(10.f),
				[this, &panel, dp = docPath, currentTag, currentElement, goTo, updatePreview](gui::GUI*) {
				createAddingUIElementPanel(panel.calcPos().x + panel.getSize().x,
					[currentElement, _dp = dp, goTo, updatePreview](const std::string& name) mutable {
					auto node = std::make_shared<xml::Node>(name);
					for (const auto& elem : uiElementsArgs.at(name).attrTemplate) {
						node->set(elem.first, elem.second);
					}
					for (const auto& elem : uiElementsArgs.at(name).elemsTemplate) {
						auto tempNode = std::make_shared<xml::Node>(elem.first);
						tempNode->set(elem.second.first, elem.second.second);
						node->add(tempNode);
					}
					currentElement->add(node);
					_dp.push_back(currentElement->getElements().size() - 1);
					updatePreview();
					goTo(_dp);
				}, (currentTag & INVENTORY ? 0 : SLOT | SLOTS_GRID));
			});

			panel += new gui::Label("Element List");
			gui::Panel& elementPanel = *new gui::Panel(glm::vec2(panel.getSize().x, panel.getSize().y / 3.f));
			elementPanel.setColor(glm::vec4(0.f));

			elementPanel.listenInterval(0.1f, [&panel, &elementPanel]() {
				const int maxLength = static_cast<int>(panel.getSize().y / 3.f);
				if (elementPanel.getMaxLength() != maxLength) {
					elementPanel.setMaxLength(maxLength);
					elementPanel.cropToContent();
				}
			});
			if (currentElement->getElements().empty())
				elementPanel += new gui::Button(L"[empty]", glm::vec4(10.f), gui::onaction());
			for (size_t i = 0; i < currentElement->getElements().size(); i++) {
				const xml::xmlelement& elem = currentElement->getElements()[i];
				std::string tag = elem->getTag();
				if (elem->has("id")) tag.append(" id: " + elem->attr("id").getText());
				elementPanel += new gui::Button(util::str2wstr_utf8(tag), glm::vec4(10.f), [i, dp = docPath, goTo](gui::GUI*) mutable {
					dp.push_back(i);
					goTo(dp);
				});
			}
			panel += elementPanel;
		}

		auto createFullCheckbox = [currentElement, &panel, updatePreview](const std::wstring& string, const std::string& attrName, bool def = true) {
			gui::FullCheckBox& checkBox = *new gui::FullCheckBox(string, glm::vec2(panel.getSize().x, 24), currentElement->attr(attrName,
				std::to_string(def)).asBool());
			checkBox.setConsumer([currentElement, updatePreview, attrName, def](bool checked) {
				if (checked == def) currentElement->removeAttr(attrName);
				else currentElement->set(attrName, std::to_string(checked));
				updatePreview();
			});
			panel += checkBox;
			return std::ref(checkBox);
		};

		auto createStatesButton = [currentElement, &panel, updatePreview](const std::wstring& buttonText, const std::string& attrName,
			std::vector<std::string> statesList) {
			statesList.emplace_back(NOT_SET);

			auto getAlign = [currentElement, attrName]() {
				return currentElement->has(attrName) ? currentElement->attr(attrName).getText() : NOT_SET;
			};

			gui::Button& button = *new gui::Button(buttonText + L": " + util::str2wstr_utf8(getAlign()), glm::vec4(10.f), gui::onaction());
			button.listenAction([&button, currentElement, getAlign, updatePreview, buttonText, attrName, statesList](gui::GUI*) {
				size_t i = std::distance(statesList.begin(), std::find(statesList.begin(), statesList.end(), getAlign())) + 1;
				if (i >= statesList.size()) i = 0;
				if (i < statesList.size() - 1) {
					currentElement->set(attrName, statesList[i]);
				}
				else {
					currentElement->removeAttr(attrName);
				}
				button.setText(buttonText + L": " + util::str2wstr_utf8(getAlign()));
				updatePreview();
			});
			panel += button;
		};

		auto createTextbox = [currentElement, &panel, updatePreview](const std::wstring& placeholder, const std::string& attrName,
			const std::function<bool(void)>& checker = 0) {
			gui::TextBox& textBox = *new gui::TextBox(placeholder);
			if (currentElement->has(attrName)) textBox.setText(util::str2wstr_utf8(currentElement->attr(attrName).getText()));
			textBox.setTextValidator([currentElement, &textBox, updatePreview, attrName, checker](const std::wstring&) {
				const std::wstring input = textBox.getInput();
				if (input.empty() || (checker && checker())) currentElement->removeAttr(attrName);
				else currentElement->set(attrName, util::wstr2str_utf8(input));
				updatePreview();
				return true;
			});
			panel += textBox;
			return std::ref(textBox);
		};

		auto createColorBox = [&panel, currentElement, updatePreview](const std::string& labelText, const std::wstring& textboxPlaceholder,
			const std::string& attrName) {
			panel += new gui::Label(labelText);
			gui::TextBox& color = *new gui::TextBox(textboxPlaceholder);
			if (currentElement->has(attrName)) color.setText(util::str2wstr_utf8(currentElement->attr(attrName).getText().substr(1)));
			color.setTextValidator([updatePreview, &color, currentElement, attrName](const std::wstring&) {
				std::string input = util::wstr2str_utf8(color.getInput());
				if (input.empty()) {
					currentElement->removeAttr(attrName);
				}
				else {
					input = '#' + input;
					try {
						xml::Attribute("", input).asColor();
					}
					catch (const std::exception&) {
						return false;
					}
					currentElement->set(attrName, input);
				}
				updatePreview();
				return true;
			});
			panel += color;
		};

		auto createVector = [currentElement, updatePreview](gui::Panel& panel, size_t vectorSize, const std::string& attrName,
			const std::vector<std::wstring>& placeholders, int min = std::numeric_limits<int>::min(), bool leaveFilled = false) {
			gui::Container& container = *new gui::Container(glm::vec2());

			std::vector<gui::TextBox*> boxes;
			const float size = panel.getSize().x / vectorSize;
			float height = 0.f;
			for (size_t i = 0; i < vectorSize; i++) {
				gui::TextBox& textBox = *new gui::TextBox(placeholders[i]);
				textBox.setSize(glm::vec2(size - textBox.getMargin().x - 4, textBox.getSize().y));
				textBox.setPos(glm::vec2(size * i, 0.f));
				height = textBox.getSize().y;
				if (currentElement->has(attrName)) {
					std::vector<std::string> strs = util::split(currentElement->attr(attrName).getText(), ',');
					textBox.setText(util::str2wstr_utf8(strs[i]));
				}
				container += boxes.emplace_back(&textBox);
			}

			for (size_t i = 0; i < boxes.size(); i++) {
				boxes[i]->setTextValidator([boxes, updatePreview, currentElement, attrName, min, leaveFilled](const std::wstring&) {
					std::wstring attrStr;
					bool success = true;
					bool allEmpty = true;
					for (size_t i = 0; i < boxes.size(); i++) {
						std::wstring input = boxes[i]->getInput();
						if (!input.empty()) allEmpty = false;
						try {
							const int num = std::stoi(input);
							if (num < min) throw std::exception();
							input = std::to_wstring(num);
							boxes[i]->setText(input);
							boxes[i]->setValid(true);
						}
						catch (const std::exception&) {
							success = false;
							if (!input.empty() && !allEmpty) allEmpty = true;
							boxes[i]->setText(leaveFilled ? util::str2wstr_utf8(currentElement->attr(attrName).getText()) : L"");
							boxes[i]->setValid(false);
						}
						attrStr.append(input + L',');
					}
					if (allEmpty) {
						for (size_t i = 0; i < boxes.size(); i++) {
							boxes[i]->setValid(true);
						}
					}
					if (success) {
						currentElement->set(attrName, util::wstr2str_utf8(attrStr.substr(0, attrStr.length() - 1)));
					}
					else if (!leaveFilled) currentElement->removeAttr(attrName);
					updatePreview();
					return true;
				});
			}

			container.setSize(glm::vec2(panel.getSize().x, height));

			panel += container;
			return std::ref(container);
		};

		auto getElementText = [](xml::xmlelement element)->std::string {
			xml::xmlelement innerText;
			for (const auto& elem : element->getElements()) {
				if (elem->getTag() == "#") {
					innerText = elem;
					break;
				}
			}
			if (innerText->has("#")) return innerText->attr("#").getText();
			return "";
		};

		// -----------------------------------------------------------------------------------------------

		if (!(currentTag & LABEL) && !(currentTag & IMAGE)) createFullCheckbox(L"Interactive", "interactive");
		if (currentTag & PANEL || currentTag & CONTAINER) createFullCheckbox(L"Scrollable", "scrollable");
		if (!root) createFullCheckbox(L"Visible", "visible");
		if (currentTag & SLOT) createFullCheckbox(L"Item source", "item-source", false);
		if (currentTag & TEXTBOX) {
			createFullCheckbox(L"Multiline", "multiline", false);
			createFullCheckbox(L"Editable", "editable");
		}
		if (currentTag & LABEL) {
			createFullCheckbox(L"Multiline", "multiline", false);
			createFullCheckbox(L"Text wrap", "text-wrap");
		}
		if (!(currentTag & SLOT) && !(currentTag & SLOTS_GRID))
			createFullCheckbox(L"Enabled", "enabled");

		panel += new gui::Label(L"Element Id");
		createTextbox(L"example_id", "id");

		if (currentTag & HAS_ElEMENT_TEXT) {
			panel += new gui::Label("Element text");
			gui::TextBox& textBox = *new gui::TextBox(L"Text");
			textBox.setText(util::str2wstr_utf8(getElementText(currentElement)));
			textBox.setTextValidator([currentElement, &textBox, updatePreview](const std::wstring&) {
				const std::wstring input = textBox.getInput();
				xml::xmlelement innerText;
				for (const auto& elem : currentElement->getElements()) {
					if (elem->getTag() == "#") {
						innerText = elem;
						break;
					}
				}
				if (input.empty()) {
					if (innerText) {
						currentElement->remove(innerText);
					}
				}
				else {
					if (!innerText) {
						innerText = std::make_shared<xml::Node>("#");
						currentElement->add(innerText);
					}
					innerText->set("#", util::wstr2str_utf8(input));
				}
				updatePreview();
				return true;
			});
			panel += textBox;
		}

		panel += new gui::Label("Z-Index (depth)");
		createVector(panel, 1, "z-index", { L"Index" });

		if (!(currentTag & SLOTS_GRID) && !(currentTag & SLOT)) {
			panel += new gui::Label("Size");
			createVector(panel, 2, "size", { L"Width", L"Height" }, 0);
		}
		if (!(currentTag & INVENTORY) && !(previousTag & PANEL)) {
			panel += new gui::Label("Position");
			createVector(panel, 2, "pos", { L"X", L"Y" });
		}
		if (!root && previousTag & PANEL) {
			panel += new gui::Label("Margin");
			createVector(panel, 4, "margin", { L"Left", L"Top", L"Right", L"Bottom" });
		}
		if (currentTag & PANEL || currentTag & BUTTON) {
			panel += new gui::Label("Padding");
			createVector(panel, 4, "padding", { L"Left", L"Top", L"Bight", L"Bottom" });
		}

		if (!root && !(previousTag & PANEL) && !(currentTag & SLOTS_GRID))
			createStatesButton(L"Element gravity", "gravity", listOfGravity);

		// -----------------------------------------------------------------------------------------------
		if (currentTag & LABEL) {
			createStatesButton(L"Horizontal align", "align", listOfHorizontalAlignments);
			createStatesButton(L"Vertical align", "valign", listOfVerticalAlignments);
		}
		if (currentTag & PANEL) {
			createStatesButton(L"Orientation", "orientation", listOfOrientations);
			panel += new gui::Label("Interval");
			createVector(panel, 1, "interval", { L"2" }, 0);
			panel += new gui::Label("Max length");
			createVector(panel, 1, "max-length", { L"Infinity" }, 0);
		}
		if (currentTag & BUTTON) {
			createStatesButton(L"Text align", "text-align", listOfHorizontalAlignments);
			panel += new gui::Label("On click function");
			createTextbox(L"empty", "onclick");
		}
		if (currentTag & SLOTS_GRID) {
			panel += new gui::Label("Start index");
			createVector(panel, 1, "start-index", { L"Index" }, 0);

			panel += new gui::Label("Slots");

			const char* const modes[] = { "rows", "cols", "count" };

			auto getMode = [currentElement, modes]() {
				unsigned int mode = 0;
				if (currentElement->has(modes[1])) {
					mode += 1;
					if (currentElement->has(modes[0])) mode += 1;
				}
				return mode;
			};

			auto getModeName = [](unsigned int mode)->std::wstring {
				if (mode == 0) return L"Rows + Count";
				else if (mode == 1) return L"Columns + Count";
				else return L"Rows + Columns";
			};

			gui::Panel& invEditorPanel = *new gui::Panel(panel.getSize());
			invEditorPanel.setColor(glm::vec4(0.f));
			auto processModeChange = [this, &invEditorPanel, modes, currentElement, createVector](unsigned int mode, bool changing = false) {
				if (changing) {
					currentElement->removeAttr(modes[0]);
					currentElement->removeAttr(modes[1]);
					currentElement->removeAttr(modes[2]);
				}
				clearRemoveList(invEditorPanel);
				if (mode == 0 || mode == 2) {
					if (changing) currentElement->set("rows", "1");
					invEditorPanel += removeList.emplace_back(new gui::Label("Rows"));
					removeList.emplace_back(&createVector(invEditorPanel, 1, "rows", { L"Rows" }, 1, true).get());
				}
				if (mode == 1 || mode == 2) {
					if (changing) currentElement->set("cols", "1");
					invEditorPanel += removeList.emplace_back(new gui::Label("Columns"));
					removeList.emplace_back(&createVector(invEditorPanel, 1, "cols", { L"Columns" }, 1, true).get());
				}
				if (mode == 0 || mode == 1) {
					if (changing) currentElement->set("count", "1");
					invEditorPanel += removeList.emplace_back(new gui::Label("Slot count"));
					removeList.emplace_back(&createVector(invEditorPanel, 1, "count", { L"Count" }, 0, true).get());
				}
				return std::ref(invEditorPanel);
			};

			gui::Button& button = *new gui::Button(L"Mode: " + getModeName(getMode()), glm::vec4(10.f), gui::onaction());
			button.listenAction([&button, getModeName, getMode, processModeChange, updatePreview](gui::GUI*) {
				unsigned int mode = getMode() + 1;
				if (mode > 2) mode = 0;
				button.setText(L"Mode: " + getModeName(mode));
				processModeChange(mode, true);
				updatePreview();
			});
			panel += button;

			panel += processModeChange(getMode());

			panel += new gui::Label("Slots interval");
			createVector(panel, 1, "interval", { L"Interval" }, 0);
		}
		if (currentTag & SLOT) {
			panel += new gui::Label("Slot index");
			createVector(panel, 1, "index", { L"Index" }, 0);
		}
		if (currentTag & TRACKBAR) {
			panel += new gui::Label("Value min");
			createVector(panel, 1, "min", { L"0" }, 0);
			panel += new gui::Label("Value max");
			createVector(panel, 1, "max", { L"1" }, 1, true);
			panel += new gui::Label("Default value");
			createVector(panel, 1, "value", { L"0" }, 0);
			panel += new gui::Label("Step");
			createVector(panel, 1, "step", { L"1" }, 1, true);
			panel += new gui::Label("Track width");
			createVector(panel, 1, "track-width", { L"1" }, 1, true);
		}
		if (currentTag & TEXTBOX) {
			panel += new gui::Label("Placeholder");
			createTextbox(L"Placeholder", "placeholder");
		}
		if (currentTag & BINDBOX) {
			panel += new gui::Label("Bind box");
			gui::TextBox& textBox = *new gui::TextBox(L"Binding");
			textBox.setText(util::str2wstr_utf8(currentElement->attr("binding").getText()));
			textBox.setTextValidator([&textBox, currentElement, updatePreview](const std::wstring&) {
				std::string input = util::wstr2str_utf8(textBox.getInput());
				if (Events::bindings.find(input) != Events::bindings.end()) {
					currentElement->set("binding", input);
					updatePreview();
					return true;
				}
				return false;
			});
			panel += textBox;
		}

		// -----------------------------------------------------------------------------------------------

		if (!(currentTag & IMAGE) && !(currentTag & SLOT) && !(currentTag & SLOTS_GRID)) {
			createColorBox("Color", L"FFFFFFFF", "color");
		}
		if (currentTag & HAS_HOVER_COLOR) {
			createColorBox("Hover color", L"", "hover-color");
		}
		if (currentTag & TEXTBOX) {
			createColorBox("Focused color", L"000000FF", "focused-color");
			createColorBox("Error color", L"190d08", "error-color");
		}
		if (currentTag & TRACKBAR) {
			createColorBox("Track color", L"FFFFFF66", "track-color");
		}
		if (currentTag & BUTTON) {
			createColorBox("Pressed color", L"000000F2", "pressed-color");
		}

		// -----------------------------------------------------------------------------------------------

		if (currentTag & HAS_CONSUMER) {
			panel += new gui::Label("Consumer function");
			createTextbox(L"empty", "consumer");
		}
		if (currentTag & HAS_SUPPLIER) {
			panel += new gui::Label("Supplier function");
			createTextbox(L"empty", "supplier");
		}
		if (currentTag & SLOT || currentTag & SLOTS_GRID) {
			panel += new gui::Label("Share function");
			createTextbox(L"empty", "sharefunc");
			panel += new gui::Label("Update function");
			createTextbox(L"empty", "updatefunc");
			panel += new gui::Label("On right click function");
			createTextbox(L"empty", "onrightclick");
		}
		if (!(currentTag & INVENTORY) && !(previousTag & PANEL)) {
			panel += new gui::Label("Position function");
			createTextbox(L"empty", "position-func");
		}
		if (currentTag & TEXTBOX) {
			panel += new gui::Label("Validator function");
			createTextbox(L"empty", "validator");
		}

		createUIPreview();

		if (previousTag & PANEL || previousTag & CONTAINER || previousTag & INVENTORY) {
			auto moveElement = [previousElement, goTo, updatePreview, dp = docPath](int offset) mutable {
				std::vector<xml::xmlelement>& elements = const_cast<std::vector<xml::xmlelement>&>(previousElement->getElements());
				size_t& index = dp.back();
				if (static_cast<int>(index) + offset < 0 || index + offset >= elements.size()) return;
				std::swap(elements[index], elements[(index + offset)]);
				index += offset;
				updatePreview();
				goTo(dp);
			};
			panel += new gui::Button(L"Move up", glm::vec4(10.f), [moveElement](gui::GUI*) mutable {
				moveElement(-1);
			});
			panel += new gui::Button(L"Move down", glm::vec4(10.f), [moveElement](gui::GUI*) mutable {
				moveElement(1);
			});
		}

		if (!root) {
			panel += new gui::Button(L"Remove current", glm::vec4(10.f), [goTo, getElement, dp = docPath, currentElement, updatePreview](gui::GUI*) mutable {
				dp.pop_back();
				xml::xmlelement elem = getElement(dp);
				elem->remove(currentElement);
				updatePreview();
				goTo(dp);
			});
		}

		panel += new gui::Button(L"Save", glm::vec4(10.f), [this, xmlDoc, actualName](gui::GUI*) {
			saveDocument(xmlDoc, currentPack.folder, actualName);
		});
		panel += new gui::Button(L"Rename", glm::vec4(10.f), [this, actualName](gui::GUI*) {
			createDefActionPanel(DefAction::RENAME, DefType::UI_LAYOUT, actualName);
		});
		panel += new gui::Button(L"Delete", glm::vec4(10.f), [this, actualName](gui::GUI*) {
			createDefActionPanel(DefAction::DELETE, DefType::UI_LAYOUT, actualName);
		});

		return std::ref(panel);
	}, 2);
}