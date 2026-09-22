// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

import { ExtensionContext } from "vscode";

import { register_debugger_support } from "./debug";
import {
  deactivate_language_client,
  start_language_client,
} from "./language_client";

const language_id = "tetrodotoxin";

export function activate(context: ExtensionContext): void {
  start_language_client(context, language_id);
  register_debugger_support(context, language_id);
}

export function deactivate(): Thenable<void> | undefined {
  return deactivate_language_client();
}
