// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

import * as path from "path";
import * as vscode from "vscode";
import { ExtensionContext } from "vscode";

const debugger_type = "lldb";

export function register_debugger_support(
  context: ExtensionContext,
  language_id: string
): void {
  // Debugger formatters are loaded only for launches that name Tetrodotoxin as
  // a source language. Existing user commands keep their authored order.
  context.subscriptions.push(
    vscode.debug.registerDebugConfigurationProvider(debugger_type, {
      resolveDebugConfiguration(
        _folder: vscode.WorkspaceFolder | undefined,
        configuration: vscode.DebugConfiguration
      ): vscode.ProviderResult<vscode.DebugConfiguration> {
        const source_languages = configuration.sourceLanguages;
        if (
          !Array.isArray(source_languages) ||
          !source_languages.includes(language_id)
        ) {
          return configuration;
        }

        const formatter_path = context.asAbsolutePath(
          path.join("lldb", "tetrodotoxin.py")
        );
        const escaped_path = formatter_path
          .replace(/\\/g, "\\\\")
          .replace(/"/g, '\\"');
        const import_command = `command script import "${escaped_path}"`;
        const configured_commands = configuration.initCommands;
        const init_commands = Array.isArray(configured_commands)
          ? [...configured_commands]
          : [];
        if (
          !init_commands.some(
            (command) =>
              typeof command === "string" &&
              command.includes("tetrodotoxin.py")
          )
        ) {
          init_commands.push(import_command);
          configuration.initCommands = init_commands;
        }
        return configuration;
      },
    })
  );
}
